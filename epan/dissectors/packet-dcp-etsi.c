/* packet-dcp-etsi.c
 * Routines for ETSI Distribution & Communication Protocol
 * Copyright 2006, British Broadcasting Corporation
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Protocol info
 * Ref: ETSI DCP (ETSI TS 102 821)
 */

#include "config.h"


#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/reassemble.h>
#include <epan/crc16-tvb.h>
#include <epan/reedsolomon.h>
#include <wsutil/array.h>

/* forward reference */
void proto_register_dcp_etsi(void);
void proto_reg_handoff_dcp_etsi(void);
static int dissect_af (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data);
static int dissect_pft (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data);

static dissector_handle_t dcp_etsi_handle;
static dissector_handle_t af_handle;
static dissector_handle_t pft_handle;
static dissector_handle_t tpl_handle;

static dissector_table_t dcp_dissector_table;
static dissector_table_t af_dissector_table;
static dissector_table_t tpl_dissector_table;

static int proto_dcp_etsi;
static int proto_af;
static int proto_pft;
static int proto_tpl;
static int hf_edcp_sync;
static int hf_edcp_len;
static int hf_edcp_seq;
static int hf_edcp_crcflag;
static int hf_edcp_maj;
static int hf_edcp_min;
static int hf_edcp_pt;
static int hf_edcp_crc;
static int hf_edcp_crc_ok;
/* static int hf_edcp_pft_pt; */
static int hf_edcp_pseq;
static int hf_edcp_findex;
static int hf_edcp_fcount;
static int hf_edcp_fecflag;
static int hf_edcp_addrflag;
static int hf_edcp_plen;
static int hf_edcp_rsk;
static int hf_edcp_rsz;
static int hf_edcp_source;
static int hf_edcp_dest;
static int hf_edcp_hcrc;
static int hf_edcp_hcrc_ok;
/* static int hf_edcp_c_max; */
/* static int hf_edcp_rx_min; */
/* static int hf_edcp_rs_corrected; */
static int hf_edcp_rs_ok;
static int hf_edcp_pft_payload;

static int hf_tpl_tlv;
/* static int hf_tpl_ptr; */

static int hf_edcp_fragments;
static int hf_edcp_fragment;
static int hf_edcp_fragment_overlap;
static int hf_edcp_fragment_overlap_conflicts;
static int hf_edcp_fragment_multiple_tails;
static int hf_edcp_fragment_too_long_fragment;
static int hf_edcp_fragment_error;
static int hf_edcp_fragment_count;
static int hf_edcp_reassembled_in;
static int hf_edcp_reassembled_length;

/* Initialize the subtree pointers */
static int ett_edcp;
static int ett_af;
static int ett_pft;
static int ett_tpl;
static int ett_edcp_fragment;
static int ett_edcp_fragments;

static expert_field ei_edcp_reassembly;
static expert_field ei_edcp_reassembly_info;

static reassembly_table dcp_reassembly_table;

static const fragment_items dcp_frag_items = {
/* Fragment subtrees */
  &ett_edcp_fragment,
  &ett_edcp_fragments,
/* Fragment fields */
  &hf_edcp_fragments,
  &hf_edcp_fragment,
  &hf_edcp_fragment_overlap,
  &hf_edcp_fragment_overlap_conflicts,
  &hf_edcp_fragment_multiple_tails,
  &hf_edcp_fragment_too_long_fragment,
  &hf_edcp_fragment_error,
  &hf_edcp_fragment_count,
/* Reassembled in field */
  &hf_edcp_reassembled_in,
/* Reassembled length field */
  &hf_edcp_reassembled_length,
/* Reassembled data field */
  NULL,
/* Tag */
  "Message fragments"
};


/** Dissect a DCP packet. Details follow
 *  here.
 *  \param[in,out] tvb The buffer containing the packet
 *  \param[in,out] pinfo The packet info structure
 *  \param[in,out] tree The structure containing the details which will be displayed, filtered, etc.
static void
 */
static int
dissect_dcp_etsi(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void * data _U_)
{
  uint8_t *sync;
  proto_tree *dcp_tree;
  proto_item *ti;

  if(tvb_captured_length(tvb) < 11)
    return false;

  /* Clear out stuff in the info column */
  col_clear(pinfo->cinfo, COL_INFO);
  col_set_str (pinfo->cinfo, COL_PROTOCOL, "DCP (ETSI)");
    /*col_append_fstr (pinfo->cinfo, COL_INFO, " tvb %d", tvb_length(tvb));*/

  ti = proto_tree_add_item (tree, proto_dcp_etsi, tvb, 0, -1, ENC_NA);
  dcp_tree = proto_item_add_subtree (ti, ett_edcp);

  sync = tvb_get_string_enc(pinfo->pool, tvb, 0, 2, ENC_ASCII);
  dissector_try_string_with_data(dcp_dissector_table, (char*)sync, tvb, pinfo, dcp_tree, true, NULL);

  return tvb_captured_length(tvb);
}

/** Heuristic dissector for a DCP packet.
 *  \param[in,out] tvb The buffer containing the packet
 *  \param[in,out] pinfo The packet info structure
 *  \param[in,out] tree The structure containing the details which will be displayed, filtered, etc.
static void
 */
static bool
dissect_dcp_etsi_heur(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void * data)
{
  /* 6.1 AF packet structure
   *
   * AF Header
   * SYNC    LEN     SEQ     AR      PT
   * 2 bytes 4 bytes 2 bytes 1 byte  1 byte
   *
   * SYNC: two-byte ASCII representation of "AF".
   * LEN: length of the payload, in bytes.
   * SEQ: sequence number
   * AR: AF protocol Revision - a field combining the CF, MAJ and MIN fields
   * CF: CRC Flag, 0 if the CRC field is not used
   * MAJ: major revision of the AF protocol in use, see clause 6.2.
   * MIN: minor revision of the AF protocol in use, see clause 6.2.
   * Protocol Type (PT): single byte encoding the protocol of the data carried in the payload.
   * For TAG Packets, the value shall be the ASCII representation of "T".
   *
   * 7.1 PFT fragment structure
   * PFT Header
   * 14, 16, 18 or 20 bytes (depending on options)                                                                              Optional present if FEC=1 Optional present if Addr = 1
   * Psync              Pseq            Findex          Fcount          FEC             HCRC            Addr    Plen    | RSk           RSz                     | Source        Dest
   * 16 bits    16 bits         24 bits         24 bits         1 bit   16 bits         1 bit   14 bits | 8 bits        8 bits          | 16 bits       16 bits
   *
   * Psync: the ASCII string "PF" is used as the synchronization word for the PFT Layer
   *
   * Don't accept this packet unless at least a full AF header present(10 bytes).
   * It should be possible to strengthen the heuristic further if need be.
   */
  uint16_t word;

  if(tvb_captured_length(tvb) < 11)
    return false;

  word = tvb_get_ntohs(tvb,0);
  /* Check for 'AF or 'PF' */
  if (word == 0x4146) {
    /* AF - check the version, which is only major 1, minor 0 */
    if ((tvb_get_uint8(tvb, 8) & 0x7F) != 0x10) {
      return false;
    }
    /* Tag packets are the only payload type */
    if (tvb_get_uint8(tvb, 9) != 'T') {
      return false;
    }
  } else if (word == 0x5046) {
    /* PFT - header length 14, 16, 18, or 20 depending on options.
     * Always contains CRC. */
    if (tvb_captured_length(tvb) < 14) {
      return false;
    }
    uint16_t plen = tvb_get_ntohs(tvb, 10);
    unsigned header_len = 14;
    if (plen & 0x8000) {
      header_len += 2;
    }
    if (plen & 0x4000) {
      header_len += 4;
    }
    if (tvb_captured_length(tvb) < header_len) {
      return false;
    }
    if (crc16_x25_ccitt_tvb(tvb, header_len) != 0x1D0F) {
      return false;
    }
  } else {
    return false;
  }

  dissect_dcp_etsi(tvb, pinfo, tree, data);

  return true;
}

#define PFT_RS_N_MAX 207
#define PFT_RS_K 255
#define PFT_RS_P (PFT_RS_K - PFT_RS_N_MAX)


static
void rs_deinterleave(const uint8_t *input, uint8_t *output, uint16_t plen, uint32_t fcount)
{
  unsigned fidx;
  for(fidx=0; fidx<fcount; fidx++)
  {
    int r;
    for (r=0; r<plen; r++)
    {
      output[fidx+r*fcount] = input[fidx*plen+r];
    }
  }
}

static
bool rs_correct_data(uint8_t *deinterleaved, uint8_t *output,
 uint32_t c_max, uint16_t rsk, uint16_t rsz _U_)
{
  uint32_t i, index_coded = 0, index_out = 0;
  int err_corr;
  for (i=0; i<c_max; i++)
  {
    memcpy(output+index_out, deinterleaved+index_coded, rsk);
    index_coded += rsk;
    memcpy(output+index_out+PFT_RS_N_MAX, deinterleaved+index_coded, PFT_RS_P);
    index_coded += PFT_RS_P;
    err_corr = eras_dec_rs(output+index_out, NULL, 0);
    if (err_corr<0) {
      return false;
    }
    index_out += rsk;
  }
  return true;
}

/* Don't attempt reassembly if we have a huge number of fragments. */
#define MAX_FRAGMENTS ((1 * 1024 * 1024) / sizeof(uint32_t))
/* If we missed more than this number of consecutive fragments,
   we don't attempt reassembly */
#define MAX_FRAG_GAP  1000

static tvbuff_t *
dissect_pft_fec_detailed(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
  uint32_t findex _U_,
  uint32_t fcount,
  uint16_t seq,
  int offset,
  uint16_t plen,
  bool fec _U_,
  uint16_t rsk,
  uint16_t rsz,
  fragment_head *fdx
)
{
  uint32_t decoded_size;
  uint32_t c_max;
  uint32_t rx_min;
  tvbuff_t *new_tvb=NULL;

  if (fcount > MAX_FRAGMENTS) {
    proto_tree_add_expert_format(tree, pinfo, &ei_edcp_reassembly, tvb , 0, -1, "[Reassembly of %d fragments not attempted]", fcount);
    return NULL;
  }

  decoded_size = fcount*plen;
  c_max = fcount*plen/(rsk+PFT_RS_P);  /* rounded down */
  rx_min = fcount - (c_max*PFT_RS_P/plen);
  if (fdx)
    new_tvb = process_reassembled_data (tvb, offset, pinfo,
                                        "Reassembled DCP (ETSI)",
                                        fdx, &dcp_frag_items,
                                        NULL, tree);
  else {
    unsigned fragments=0;
    uint32_t *got;
    fragment_item *fd;
    fragment_head *fd_head;

    proto_tree_add_expert_format(tree, pinfo, &ei_edcp_reassembly_info, tvb, 0, -1, "want %d, got %d need %d",
                           fcount, fragments, rx_min);
    got = (uint32_t *)wmem_alloc(pinfo->pool, fcount*sizeof(uint32_t));

    /* make a list of the findex (offset) numbers of the fragments we have */
    fd_head = fragment_get(&dcp_reassembly_table, pinfo, seq, NULL);
    if (fd_head) {
      for (fd = fd_head->next; fd != NULL && fragments < fcount; fd = fd->next) {
        if(fd->tvb_data) {
          got[fragments++] = fd->offset; /* this is the findex of the fragment */
        }
      }
    }
    /* have we got enough for Reed Solomon to try to correct ? */
    if(fragments>=rx_min) { /* yes, in theory */
      unsigned i,current_findex;
      fragment_head *frag=NULL;
      uint8_t *dummy_data = (uint8_t*) wmem_alloc0 (pinfo->pool, plen);
      tvbuff_t *dummytvb = tvb_new_real_data(dummy_data, plen, plen);
      /* try and decode with missing fragments */
      proto_tree_add_expert_format(tree, pinfo, &ei_edcp_reassembly_info, tvb, 0, -1, "want %d, got %d need %d",
                               fcount, fragments, rx_min);
      /* fill the fragment table with empty fragments */
      current_findex = 0;
      for(i=0; i<fragments; i++) {
        unsigned next_fragment_we_have = got[i];
        if (next_fragment_we_have > MAX_FRAGMENTS) {
          proto_tree_add_expert_format(tree, pinfo, &ei_edcp_reassembly, tvb , 0, -1, "[Reassembly of %d fragments not attempted]", next_fragment_we_have);
          return NULL;
        }
        if (next_fragment_we_have-current_findex > MAX_FRAG_GAP) {
          proto_tree_add_expert_format(tree, pinfo, &ei_edcp_reassembly, tvb, 0, -1,
              "[Missing %d consecutive packets. Don't attempt reassembly]",
              next_fragment_we_have-current_findex);
          return NULL;
        }
        for(; current_findex<next_fragment_we_have; current_findex++) {
          frag = fragment_add_seq_check (&dcp_reassembly_table,
                                         dummytvb, 0, pinfo, seq, NULL,
                                         current_findex, plen, (current_findex+1!=fcount));
        }
        current_findex++; /* skip over the fragment we have */
      }
      tvb_free(dummytvb);

      if(frag)
        new_tvb = process_reassembled_data (tvb, offset, pinfo,
                                            "Reassembled DCP (ETSI)",
                                            frag, &dcp_frag_items,
                                            NULL, tree);
    }
  }
  if(new_tvb && tvb_captured_length(new_tvb) > 0) {
    bool decoded;
    tvbuff_t *dtvb = NULL;
    const uint8_t *input = tvb_get_ptr(new_tvb, 0, -1);
    uint32_t reassembled_size = tvb_captured_length(new_tvb);
    uint8_t *deinterleaved = (uint8_t*) wmem_alloc(pinfo->pool, reassembled_size);
    uint8_t *output = (uint8_t*) wmem_alloc(pinfo->pool, decoded_size);
    rs_deinterleave(input, deinterleaved, plen, fcount);

    dtvb = tvb_new_child_real_data(tvb, deinterleaved, reassembled_size, reassembled_size);
    add_new_data_source(pinfo, dtvb, "Deinterleaved");

    decoded = rs_correct_data(deinterleaved, output, c_max, rsk, rsz);
    proto_tree_add_boolean (tree, hf_edcp_rs_ok, tvb, offset, 2, decoded);

    new_tvb = tvb_new_child_real_data(dtvb, output, decoded_size, decoded_size);
    add_new_data_source(pinfo, new_tvb, "RS Error Corrected Data");
  }
  return new_tvb;
}


/** Handle a PFT packet which has the fragmentation header. This uses the
 * standard wireshark methods for reassembling fragments. If FEC is used,
 * the FEC is handled too. For the moment, all the fragments must be
 * available but this could be improved.
 *  \param[in,out] tvb The buffer containing the current fragment
 *  \param[in,out] pinfo The packet info structure
 *  \param[in,out] tree The structure containing the details which will be displayed, filtered, etc.
 *  \param[in] findex the fragment count
 *  \param[in] fcount the number of fragments
 *  \param[in] seq the sequence number of the reassembled packet
 *  \param[in] offset the offset into the tvb of the fragment
 *  \param[in] plen the length of each fragment
 *  \param[in] fec is fec used
 *  \param[in] rsk the number of useful bytes in each chunk
 *  \param[in] rsz the number of padding bytes in each chunk
 */
static tvbuff_t *
dissect_pft_fragmented(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree,
  uint32_t findex,
  uint32_t fcount,
  uint16_t seq,
  int offset,
  uint16_t plen,
  bool fec,
  uint16_t rsk,
  uint16_t rsz
)
{
  bool first, last;
  tvbuff_t *new_tvb=NULL;
  fragment_head *frag_edcp = NULL;
  pinfo->fragmented = true;
  first = findex == 0;
  last = fcount == (findex+1);
  frag_edcp = fragment_add_seq_check (
    &dcp_reassembly_table,
    tvb, offset,
    pinfo, seq, NULL,
    findex,
    plen,
    !last);
  if(fec) {
    new_tvb = dissect_pft_fec_detailed(
      tvb, pinfo, tree, findex, fcount, seq, offset, plen, fec, rsk, rsz, frag_edcp
      );
  } else {
    new_tvb = process_reassembled_data (tvb, offset, pinfo,
                                        "Reassembled DCP (ETSI)",
                                        frag_edcp, &dcp_frag_items,
                                        NULL, tree);
  }
  if(new_tvb) {
    col_append_str (pinfo->cinfo, COL_INFO, " (Message Reassembled)");
  } else {
    if(last) {
      col_append_str (pinfo->cinfo, COL_INFO, " (Message Reassembly failure)");
    } else {
      col_append_fstr (pinfo->cinfo, COL_INFO, " (Message fragment %u)", findex);
    }
  }
  if(first)
    col_append_str (pinfo->cinfo, COL_INFO, " (first)");
  if(last)
   col_append_str (pinfo->cinfo, COL_INFO, " (last)");
  return new_tvb;
  }

/** Dissect a PFT packet. Details follow
 *  here.
 *  \param[in,out] tvb The buffer containing the packet
 *  \param[in,out] pinfo The packet info structure
 *  \param[in,out] tree The structure containing the details which will be displayed, filtered, etc.
 */
static int
dissect_pft(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data)
{
  uint16_t plen;
  int offset = 0;
  uint16_t seq, payload_len;
  uint32_t findex, fcount;
  proto_tree *pft_tree;
  proto_item *ti, *li;
  tvbuff_t *next_tvb = NULL;
  bool fec = false;
  uint16_t rsk=0, rsz=0;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "DCP-PFT");

  ti = proto_tree_add_item (tree, proto_pft, tvb, 0, -1, ENC_NA);
  pft_tree = proto_item_add_subtree (ti, ett_pft);
  proto_tree_add_item (pft_tree, hf_edcp_sync, tvb, offset, 2, ENC_ASCII);

  offset += 2;
  seq = tvb_get_ntohs (tvb, offset);
  proto_tree_add_item (pft_tree, hf_edcp_pseq, tvb, offset, 2, ENC_BIG_ENDIAN);

  offset += 2;
  findex = tvb_get_ntoh24 (tvb, offset);
  proto_tree_add_item (pft_tree, hf_edcp_findex, tvb, offset, 3, ENC_BIG_ENDIAN);

  offset += 3;
  fcount = tvb_get_ntoh24 (tvb, offset);
  proto_tree_add_item (pft_tree, hf_edcp_fcount, tvb, offset, 3, ENC_BIG_ENDIAN);

  offset += 3;
  plen = tvb_get_ntohs (tvb, offset);
  payload_len = plen & 0x3fff;
  proto_tree_add_item (pft_tree, hf_edcp_fecflag, tvb, offset, 2, ENC_BIG_ENDIAN);
  proto_tree_add_item (pft_tree, hf_edcp_addrflag, tvb, offset, 2, ENC_BIG_ENDIAN);
  li = proto_tree_add_item (pft_tree, hf_edcp_plen, tvb, offset, 2, ENC_BIG_ENDIAN);

  offset += 2;
  if (plen & 0x8000) {
    fec = true;
    rsk = tvb_get_uint8 (tvb, offset);
    proto_tree_add_item (pft_tree, hf_edcp_rsk, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
    rsz = tvb_get_uint8 (tvb, offset);
    proto_tree_add_item (pft_tree, hf_edcp_rsz, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset += 1;
  }
  if (plen & 0x4000) {
    proto_tree_add_item (pft_tree, hf_edcp_source, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;
    proto_tree_add_item (pft_tree, hf_edcp_dest, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset += 2;
  }
  if (tree) {
    proto_item *ci = NULL;
    unsigned header_len = offset+2;
    uint16_t c = crc16_x25_ccitt_tvb(tvb, header_len);
    ci = proto_tree_add_item (pft_tree, hf_edcp_hcrc, tvb, offset, 2, ENC_BIG_ENDIAN);
    proto_item_append_text(ci, " (%s)", (c==0x1D0F)?"Ok":"bad");
    proto_tree_add_boolean(pft_tree, hf_edcp_hcrc_ok, tvb, offset, 2, c==0x1D0F);
  }
  offset += 2;
  if (fcount > 1) {             /* fragmented*/
    bool save_fragmented = pinfo->fragmented;
    uint16_t real_len = tvb_captured_length(tvb)-offset;
    proto_tree_add_item (pft_tree, hf_edcp_pft_payload, tvb, offset, real_len, ENC_NA);
    if(real_len != payload_len || real_len == 0) {
      proto_item_append_text(li, " (length error (%d))", real_len);
    }
    else {
      next_tvb = dissect_pft_fragmented(tvb, pinfo, pft_tree, findex, fcount,
                                        seq, offset, real_len, fec, rsk, rsz);
    }
    pinfo->fragmented = save_fragmented;
  } else {
    next_tvb = tvb_new_subset_remaining (tvb, offset);
  }
  if(next_tvb) {
    dissect_af(next_tvb, pinfo, tree, data);
  }
  return tvb_captured_length(tvb);
}

/** Dissect an AF Packet. Parse an AF packet, checking the CRC if the CRC valid
 * flag is set and calling any registered sub dissectors on the payload type.
 * Currently only a payload type 'T' is defined which is the tag packet layer.
 * If any others are defined then they can register themselves.
 *  \param[in,out] tvb The buffer containing the packet
 *  \param[in,out] pinfo The packet info structure
 *  \param[in,out] tree The structure containing the details which will be displayed, filtered, etc.
 */
static int
dissect_af (tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
  int offset = 0;
  proto_item *ti;
  proto_item *li = NULL;
  proto_item *ci;
  proto_tree *af_tree;
  uint8_t ver, pt;
  uint32_t payload_len;
  tvbuff_t *next_tvb = NULL;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "DCP-AF");

  ti = proto_tree_add_item (tree, proto_af, tvb, 0, -1, ENC_NA);
  af_tree = proto_item_add_subtree (ti, ett_af);
  proto_tree_add_item (af_tree, hf_edcp_sync, tvb, offset, 2, ENC_ASCII);

  offset += 2;
  payload_len = tvb_get_ntohl(tvb, offset);
  if (tree) {
    uint32_t real_payload_len = tvb_captured_length(tvb)-12;
    li = proto_tree_add_item (af_tree, hf_edcp_len, tvb, offset, 4, ENC_BIG_ENDIAN);
    if(real_payload_len < payload_len) {
      proto_item_append_text (li, " (wrong len claims %d is %d)",
      payload_len, real_payload_len
      );
    } else if(real_payload_len > payload_len) {
      proto_item_append_text (li, " (%d bytes in packet after end of AF frame)",
      real_payload_len-payload_len
      );
    }
  }
  offset += 4;
  proto_tree_add_item (af_tree, hf_edcp_seq, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;
  ver = tvb_get_uint8 (tvb, offset);
  proto_tree_add_item (af_tree, hf_edcp_crcflag, tvb, offset, 1, ENC_BIG_ENDIAN);
  proto_tree_add_item (af_tree, hf_edcp_maj, tvb, offset, 1, ENC_BIG_ENDIAN);
  proto_tree_add_item (af_tree, hf_edcp_min, tvb, offset, 1, ENC_BIG_ENDIAN);

  offset += 1;
  pt = tvb_get_uint8 (tvb, offset);
  proto_tree_add_item (af_tree, hf_edcp_pt, tvb, offset, 1, ENC_ASCII);
  offset += 1;
  next_tvb = tvb_new_subset_length(tvb, offset, payload_len);
  offset += payload_len;
  ci = proto_tree_add_item (af_tree, hf_edcp_crc, tvb, offset, 2, ENC_BIG_ENDIAN);
  if (ver & 0x80) { /* crc valid */
    unsigned len = offset+2;
    uint16_t c = crc16_x25_ccitt_tvb(tvb, len);
    proto_item_append_text(ci, " (%s)", (c==0x1D0F)?"Ok":"bad");
    proto_tree_add_boolean(af_tree, hf_edcp_crc_ok, tvb, offset, 2, c==0x1D0F);
  }
  /*offset += 2;*/

  dissector_try_uint(af_dissector_table, pt, next_tvb, pinfo, tree);
  return tvb_captured_length(tvb);
}

/** Dissect the Tag Packet Layer.
 *  Split the AF packet into its tag items. Each tag item has a 4 character
 *  tag, a length in bits and a value. The *ptr tag is dissected in the routine.
 *  All other tags are listed and may be handled by other dissectors.
 *  Child dissectors are tied to the parent tree, not to this tree, so that
 *  they appear at the same level as DCP.
 *  \param[in,out] tvb The buffer containing the packet
 *  \param[in,out] pinfo The packet info structure
 *  \param[in,out] tree The structure containing the details which will be displayed, filtered, etc.
 */
static int
dissect_tpl(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
  proto_tree *tpl_tree;
  unsigned offset=0;
  proto_item *ti;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "DCP-TPL");

  ti = proto_tree_add_item (tree, proto_tpl, tvb, 0, -1, ENC_NA);
  tpl_tree = proto_item_add_subtree (ti, ett_tpl);

  while(offset<tvb_reported_length(tvb)) {
    tvbuff_t *next_tvb;
    uint32_t bits;
    uint32_t bytes;
    char *tag = (char*)tvb_get_string_enc(pinfo->pool, tvb, offset, 4, ENC_ASCII);
    bits = tvb_get_ntohl(tvb, offset+4);
    bytes = bits / 8;
    if(bits % 8)
      bytes++;

    proto_tree_add_bytes_format(tpl_tree, hf_tpl_tlv, tvb,
            offset, 8+bytes, NULL,
            "%s (%u bits)", tag, bits);

    next_tvb = tvb_new_subset_length(tvb, offset+8, bytes);
    dissector_try_string_with_data(tpl_dissector_table, tag, next_tvb, pinfo, tree, true, NULL);

    offset += (8+bytes);
  }

  return tvb_captured_length(tvb);
}

void
proto_reg_handoff_dcp_etsi (void)
{
  heur_dissector_add("udp", dissect_dcp_etsi_heur, "DCP (ETSI) over UDP", "dcp_etsi_udp", proto_dcp_etsi, HEURISTIC_ENABLE);
  dissector_add_for_decode_as("udp.port", dcp_etsi_handle);
  dissector_add_string("dcp-etsi.sync", "AF", af_handle);
  dissector_add_string("dcp-etsi.sync", "PF", pft_handle);
  /* if there are ever other payload types ...*/
  dissector_add_uint("dcp-af.pt", 'T', tpl_handle);
}

void
proto_register_dcp_etsi (void)
{
  static hf_register_info hf_edcp[] = {
    {&hf_edcp_sync,
     {"sync", "dcp-etsi.sync",
      FT_STRING, BASE_NONE, NULL, 0,
      "AF or PF", HFILL}
     }
    };
  static hf_register_info hf_af[] = {
    {&hf_edcp_len,
     {"length", "dcp-af.len",
      FT_UINT32, BASE_DEC, NULL, 0,
      "length in bytes of the payload", HFILL}
     },
    {&hf_edcp_seq,
     {"frame count", "dcp-af.seq",
      FT_UINT16, BASE_DEC, NULL, 0,
      "Logical Frame Number", HFILL}
     },
    {&hf_edcp_crcflag,
     {"crc flag", "dcp-af.crcflag",
      FT_BOOLEAN, 8, NULL, 0x80,
      "Frame is protected by CRC", HFILL}
     },
    {&hf_edcp_maj,
     {"Major Revision", "dcp-af.maj",
      FT_UINT8, BASE_DEC, NULL, 0x70,
      "Major Protocol Revision", HFILL}
     },
    {&hf_edcp_min,
     {"Minor Revision", "dcp-af.min",
      FT_UINT8, BASE_DEC, NULL, 0x0f,
      "Minor Protocol Revision", HFILL}
     },
    {&hf_edcp_pt,
     {"Payload Type", "dcp-af.pt",
      FT_STRING, BASE_NONE, NULL, 0,
      "T means Tag Packets, all other values reserved", HFILL}
     },
    {&hf_edcp_crc,
     {"CRC", "dcp-af.crc",
      FT_UINT16, BASE_HEX, NULL, 0,
      NULL, HFILL}
     },
    {&hf_edcp_crc_ok,
     {"CRC OK", "dcp-af.crc_ok",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
      "AF CRC OK", HFILL}
     }
    };

  static hf_register_info hf_pft[] = {
#if 0
    {&hf_edcp_pft_pt,
     {"Sub-protocol", "dcp-pft.pt",
      FT_UINT8, BASE_DEC, NULL, 0,
      "Always AF", HFILL}
     },
#endif
    {&hf_edcp_pseq,
     {"Sequence No", "dcp-pft.seq",
      FT_UINT16, BASE_DEC, NULL, 0,
      "PFT Sequence No", HFILL}
     },
    {&hf_edcp_findex,
     {"Fragment Index", "dcp-pft.findex",
      FT_UINT24, BASE_DEC, NULL, 0,
      "Index of the fragment within one AF Packet", HFILL}
     },
    {&hf_edcp_fcount,
     {"Fragment Count", "dcp-pft.fcount",
      FT_UINT24, BASE_DEC, NULL, 0,
      "Number of fragments produced from this AF Packet", HFILL}
     },
    {&hf_edcp_fecflag,
     {"FEC", "dcp-pft.fec",
      FT_BOOLEAN, 16, NULL, 0x8000,
      "When set the optional RS header is present", HFILL}
     },
    {&hf_edcp_addrflag,
     {"Addr", "dcp-pft.addr",
      FT_BOOLEAN, 16, NULL, 0x4000,
      "When set the optional transport header is present", HFILL}
     },
    {&hf_edcp_plen,
     {"fragment length", "dcp-pft.len",
      FT_UINT16, BASE_DEC, NULL, 0x3fff,
      "length in bytes of the payload of this fragment", HFILL}
     },
    {&hf_edcp_rsk,
     {"RSk", "dcp-pft.rsk",
      FT_UINT8, BASE_DEC, NULL, 0,
      "The length of the Reed Solomon data word", HFILL}
     },
    {&hf_edcp_rsz,
     {"RSz", "dcp-pft.rsz",
      FT_UINT8, BASE_DEC, NULL, 0,
      "The number of padding bytes in the last Reed Solomon block", HFILL}
     },
    {&hf_edcp_source,
     {"source addr", "dcp-pft.source",
      FT_UINT16, BASE_DEC, NULL, 0,
      "PFT source identifier", HFILL}
     },
    {&hf_edcp_dest,
     {"dest addr", "dcp-pft.dest",
      FT_UINT16, BASE_DEC, NULL, 0,
      "PFT destination identifier", HFILL}
     },
    {&hf_edcp_hcrc,
     {"header CRC", "dcp-pft.crc",
      FT_UINT16, BASE_HEX, NULL, 0,
      "PFT Header CRC", HFILL}
     },
    {&hf_edcp_hcrc_ok,
     {"PFT CRC OK", "dcp-pft.crc_ok",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
      "PFT Header CRC OK", HFILL}
     },
    {&hf_edcp_fragments,
     {"Message fragments", "dcp-pft.fragments",
      FT_NONE, BASE_NONE, NULL, 0x00, NULL, HFILL}},
    {&hf_edcp_fragment,
     {"Message fragment", "dcp-pft.fragment",
      FT_FRAMENUM, BASE_NONE, NULL, 0x00, NULL, HFILL}},
    {&hf_edcp_fragment_overlap,
     {"Message fragment overlap", "dcp-pft.fragment.overlap",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0, NULL, HFILL}},
    {&hf_edcp_fragment_overlap_conflicts,
     {"Message fragment overlapping with conflicting data",
      "dcp-pft.fragment.overlap.conflicts",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0, NULL, HFILL}},
    {&hf_edcp_fragment_multiple_tails,
     {"Message has multiple tail fragments",
      "dcp-pft.fragment.multiple_tails",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0, NULL, HFILL}},
    {&hf_edcp_fragment_too_long_fragment,
     {"Message fragment too long", "dcp-pft.fragment.too_long_fragment",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0, NULL, HFILL}},
    {&hf_edcp_fragment_error,
     {"Message defragmentation error", "dcp-pft.fragment.error",
      FT_FRAMENUM, BASE_NONE, NULL, 0x00, NULL, HFILL}},
    {&hf_edcp_fragment_count,
     {"Message fragment count", "dcp-pft.fragment.count",
      FT_UINT32, BASE_DEC, NULL, 0x00, NULL, HFILL}},
    {&hf_edcp_reassembled_in,
     {"Reassembled in", "dcp-pft.reassembled.in",
      FT_UINT32, BASE_DEC, NULL, 0x00, NULL, HFILL}},
    {&hf_edcp_reassembled_length,
     {"Reassembled DCP (ETSI) length", "dcp-pft.reassembled.length",
      FT_UINT32, BASE_DEC, NULL, 0x00, NULL, HFILL}},
#if 0
    {&hf_edcp_c_max,
     {"C max", "dcp-pft.cmax",
      FT_UINT16, BASE_DEC, NULL, 0,
      "Maximum number of RS chunks sent", HFILL}
     },
    {&hf_edcp_rx_min,
     {"Rx min", "dcp-pft.rxmin",
      FT_UINT16, BASE_DEC, NULL, 0,
      "Minimum number of fragments needed for RS decode", HFILL}
     },
    {&hf_edcp_rs_corrected,
     {"RS Symbols Corrected", "dcp-pft.rs_corrected",
      FT_INT16, BASE_DEC, NULL, 0,
      "Number of symbols corrected by RS decode or -1 for failure", HFILL}
     },
#endif
    {&hf_edcp_rs_ok,
     {"RS decode OK", "dcp-pft.rs_ok",
      FT_BOOLEAN, BASE_NONE, NULL, 0x0,
      "successfully decoded RS blocks", HFILL}
     },
    {&hf_edcp_pft_payload,
     {"payload", "dcp-pft.payload",
      FT_BYTES, BASE_NONE, NULL, 0,
      "PFT Payload", HFILL}
    }
  };

  static hf_register_info hf_tpl[] = {
    {&hf_tpl_tlv,
     {"tag", "dcp-tpl.tlv",
      FT_BYTES, BASE_NONE, NULL, 0,
      "Tag Packet", HFILL}
     },
#if 0
    {&hf_tpl_ptr,
     {"Type", "dcp-tpl.ptr",
      FT_STRING, BASE_NONE, NULL, 0,
      "Protocol Type & Revision", HFILL}
     }
#endif
    };

/* Setup protocol subtree array */
  static int *ett[] = {
    &ett_edcp,
    &ett_af,
    &ett_pft,
    &ett_tpl,
    &ett_edcp_fragment,
    &ett_edcp_fragments
  };

  static ei_register_info ei[] = {
     { &ei_edcp_reassembly, { "dcp-etsi.reassembly_failed", PI_REASSEMBLE, PI_ERROR, "Reassembly failed", EXPFILL }},
     { &ei_edcp_reassembly_info, { "dcp-etsi.reassembly_info", PI_REASSEMBLE, PI_CHAT, "Reassembly information", EXPFILL }},
  };

  expert_module_t* expert_dcp_etsi;

  proto_dcp_etsi = proto_register_protocol ("ETSI Distribution & Communication Protocol (for DRM)",     /* name */
                                            "DCP (ETSI)",       /* short name */
                                            "dcp-etsi"  /* abbrev */
    );
  proto_af = proto_register_protocol ("DCP Application Framing Layer", "DCP-AF", "dcp-af");
  proto_pft = proto_register_protocol ("DCP Protection, Fragmentation & Transport Layer", "DCP-PFT", "dcp-pft");
  proto_tpl = proto_register_protocol ("DCP Tag Packet Layer", "DCP-TPL", "dcp-tpl");

  proto_register_field_array (proto_dcp_etsi, hf_edcp, array_length (hf_edcp));
  proto_register_field_array (proto_af, hf_af, array_length (hf_af));
  proto_register_field_array (proto_pft, hf_pft, array_length (hf_pft));
  proto_register_field_array (proto_tpl, hf_tpl, array_length (hf_tpl));
  proto_register_subtree_array (ett, array_length (ett));
  expert_dcp_etsi = expert_register_protocol(proto_dcp_etsi);
  expert_register_field_array(expert_dcp_etsi, ei, array_length(ei));

  /* subdissector code */
  dcp_dissector_table = register_dissector_table("dcp-etsi.sync",
            "DCP Sync", proto_dcp_etsi, FT_STRING, STRING_CASE_SENSITIVE);
  af_dissector_table = register_dissector_table("dcp-af.pt",
            "DCP-AF Payload Type", proto_dcp_etsi, FT_UINT8, BASE_DEC);

  tpl_dissector_table = register_dissector_table("dcp-tpl.ptr",
            "DCP-TPL Protocol Type & Revision", proto_dcp_etsi, FT_STRING, STRING_CASE_SENSITIVE);

  reassembly_table_register (&dcp_reassembly_table,
                         &addresses_reassembly_table_functions);

  dcp_etsi_handle = register_dissector("dcp-etsi", dissect_dcp_etsi, proto_dcp_etsi);
  af_handle = register_dissector("dcp-af", dissect_af, proto_af);
  pft_handle = register_dissector("dcp-pft", dissect_pft, proto_pft);
  tpl_handle = register_dissector("dcp-tpl", dissect_tpl, proto_tpl);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
