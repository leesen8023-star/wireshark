/* Do not modify this file. Changes will be overwritten.                      */
/* Generated automatically by the ASN.1 to Wireshark dissector compiler       */
/* packet-spnego.c                                                            */
/* asn2wrs.py -b -q -L -p spnego -c ./spnego.cnf -s ./packet-spnego-template -D . -O ../.. spnego.asn */

/* packet-spnego-template.c
 * Routines for the simple and protected GSS-API negotiation mechanism
 * as described in RFC 2478.
 * Copyright 2002, Tim Potter <tpot@samba.org>
 * Copyright 2002, Richard Sharpe <rsharpe@ns.aus.com>
 * Copyright 2003, Richard Sharpe <rsharpe@richardsharpe.com>
 * Copyright 2005, Ronnie Sahlberg (krb decryption)
 * Copyright 2005, Anders Broman (converted to asn2wrs generated dissector)
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
/* The heimdal code for decryption of GSSAPI wrappers using heimdal comes from
   Heimdal 1.6 and has been modified for wireshark's requirements.
*/

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/asn1.h>
#include <epan/conversation.h>
#include <epan/proto_data.h>
#include <epan/tfs.h>
#include <wsutil/wsgcrypt.h>
#include <wsutil/array.h>
#include "packet-gssapi.h"
#include "packet-kerberos.h"
#include "packet-ber.h"

#define PNAME  "Simple Protected Negotiation"
#define PSNAME "SPNEGO"
#define PFNAME "spnego"

void proto_register_spnego(void);
void proto_reg_handoff_spnego(void);

static dissector_handle_t spnego_wrap_handle;

/* Initialize the protocol and registered fields */
static int proto_spnego;
static int proto_spnego_krb5;


static int hf_spnego_wraptoken;
static int hf_spnego_krb5_oid;
static int hf_spnego_krb5;
static int hf_spnego_krb5_tok_id;
static int hf_spnego_krb5_sgn_alg;
static int hf_spnego_krb5_seal_alg;
static int hf_spnego_krb5_snd_seq;
static int hf_spnego_krb5_sgn_cksum;
static int hf_spnego_krb5_confounder;
static int hf_spnego_krb5_filler;
static int hf_spnego_krb5_cfx_flags;
static int hf_spnego_krb5_cfx_flags_01;
static int hf_spnego_krb5_cfx_flags_02;
static int hf_spnego_krb5_cfx_flags_04;
static int hf_spnego_krb5_cfx_ec;
static int hf_spnego_krb5_cfx_rrc;
static int hf_spnego_krb5_cfx_seq;

static int hf_spnego_negTokenInit;                /* T_negTokenInit */
static int hf_spnego_negTokenTarg;                /* NegTokenTarg */
static int hf_spnego_MechTypeList_item;           /* MechType */
static int hf_spnego_mechTypes;                   /* MechTypeList */
static int hf_spnego_reqFlags;                    /* ContextFlags */
static int hf_spnego_mechToken;                   /* T_mechToken */
static int hf_spnego_mechListMIC;                 /* OCTET_STRING */
static int hf_spnego_hintName;                    /* GeneralString */
static int hf_spnego_hintAddress;                 /* OCTET_STRING */
static int hf_spnego_mechToken_01;                /* OCTET_STRING */
static int hf_spnego_negHints;                    /* NegHints */
static int hf_spnego_negResult;                   /* T_negResult */
static int hf_spnego_supportedMech;               /* T_supportedMech */
static int hf_spnego_responseToken;               /* T_responseToken */
static int hf_spnego_mechListMIC_01;              /* T_mechListMIC */
static int hf_spnego_thisMech;                    /* MechType */
static int hf_spnego_innerContextToken;           /* InnerContextToken */
static int hf_spnego_target_realm;                /* T_target_realm */
static int hf_spnego_cookie;                      /* OCTET_STRING */
static int hf_spnego_header_flags;                /* HeaderFlags */
/* named bits */
static int hf_spnego_ContextFlags_delegFlag;
static int hf_spnego_ContextFlags_mutualFlag;
static int hf_spnego_ContextFlags_replayFlag;
static int hf_spnego_ContextFlags_sequenceFlag;
static int hf_spnego_ContextFlags_anonFlag;
static int hf_spnego_ContextFlags_confFlag;
static int hf_spnego_ContextFlags_integFlag;
static int hf_spnego_HeaderFlags_unused0;
static int hf_spnego_HeaderFlags_return_dns_name;
static int hf_spnego_HeaderFlags_unused2;
static int hf_spnego_HeaderFlags_unused3;
static int hf_spnego_HeaderFlags_unused4;
static int hf_spnego_HeaderFlags_ds_12_required;
static int hf_spnego_HeaderFlags_ds_13_required;
static int hf_spnego_HeaderFlags_key_list_support_required;
static int hf_spnego_HeaderFlags_ds_10_required;
static int hf_spnego_HeaderFlags_ds_9_required;
static int hf_spnego_HeaderFlags_ds_8_required;
static int hf_spnego_HeaderFlags_web_service_required;
static int hf_spnego_HeaderFlags_ds_6_required;
static int hf_spnego_HeaderFlags_try_next_closest_site;
static int hf_spnego_HeaderFlags_is_dns_name;
static int hf_spnego_HeaderFlags_is_flat_name;
static int hf_spnego_HeaderFlags_only_ldap_needed;
static int hf_spnego_HeaderFlags_avoid_self;
static int hf_spnego_HeaderFlags_good_timeserv_pref;
static int hf_spnego_HeaderFlags_writable_required;
static int hf_spnego_HeaderFlags_timeserv_required;
static int hf_spnego_HeaderFlags_kdc_required;
static int hf_spnego_HeaderFlags_ip_required;
static int hf_spnego_HeaderFlags_background_only;
static int hf_spnego_HeaderFlags_pdc_required;
static int hf_spnego_HeaderFlags_gc_server_required;
static int hf_spnego_HeaderFlags_ds_preferred;
static int hf_spnego_HeaderFlags_ds_required;
static int hf_spnego_HeaderFlags_unused28;
static int hf_spnego_HeaderFlags_unused29;
static int hf_spnego_HeaderFlags_unused30;
static int hf_spnego_HeaderFlags_force_rediscovery;

/* Global variables */
static const char *MechType_oid;
gssapi_oid_value *next_level_value;
bool saw_mechanism;


/* Initialize the subtree pointers */
static int ett_spnego;
static int ett_spnego_wraptoken;
static int ett_spnego_krb5;
static int ett_spnego_krb5_cfx_flags;

static int ett_spnego_NegotiationToken;
static int ett_spnego_MechTypeList;
static int ett_spnego_NegTokenInit;
static int ett_spnego_NegHints;
static int ett_spnego_NegTokenInit2;
static int ett_spnego_ContextFlags;
static int ett_spnego_NegTokenTarg;
static int ett_spnego_InitialContextToken_U;
static int ett_spnego_HeaderFlags;
static int ett_spnego_IAKERB_HEADER;

static expert_field ei_spnego_decrypted_keytype;
static expert_field ei_spnego_unknown_header;

static dissector_handle_t spnego_handle;
static dissector_handle_t spnego_krb5_handle;
static dissector_handle_t spnego_krb5_wrap_handle;

/*
 * Unfortunately, we have to have forward declarations of these,
 * as the code generated by asn2wrs includes a call before the
 * definition.
 */
static int dissect_spnego_NegTokenInit(bool implicit_tag, tvbuff_t *tvb,
                                       int offset, asn1_ctx_t *actx _U_,
                                       proto_tree *tree, int hf_index);
static int dissect_spnego_NegTokenInit2(bool implicit_tag, tvbuff_t *tvb,
                                        int offset, asn1_ctx_t *actx _U_,
                                        proto_tree *tree, int hf_index);



static int
dissect_spnego_MechType(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  gssapi_oid_value *value;

  offset = dissect_ber_object_identifier_str(implicit_tag, actx, tree, tvb, offset, hf_index, &MechType_oid);


  value = gssapi_lookup_oid_str(MechType_oid);

  /*
   * Tell our caller the first mechanism we see, so that if
   * this is a negTokenInit with a mechToken, it can interpret
   * the mechToken according to the first mechType.  (There
   * might not have been any indication of the mechType
   * in prior frames, so we can't necessarily use the
   * mechanism from the conversation; i.e., a negTokenInit
   * can contain the initial security token for the desired
   * mechanism of the initiator - that's the first mechanism
   * in the list.)
   */
  if (!saw_mechanism) {
    if (value)
      next_level_value = value;
    saw_mechanism = true;
  }


  return offset;
}


static const ber_sequence_t MechTypeList_sequence_of[1] = {
  { &hf_spnego_MechTypeList_item, BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_spnego_MechType },
};

static int
dissect_spnego_MechTypeList(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  conversation_t *conversation;

  saw_mechanism = false;

  offset = dissect_ber_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                      MechTypeList_sequence_of, hf_index, ett_spnego_MechTypeList);


  /*
   * If we saw a mechType we need to store it in case the negTokenTarg
   * does not provide a supportedMech.
   */
  if(saw_mechanism){
    conversation = find_or_create_conversation(actx->pinfo);
    conversation_add_proto_data(conversation, proto_spnego, next_level_value);
  }


  return offset;
}


static int * const ContextFlags_bits[] = {
  &hf_spnego_ContextFlags_delegFlag,
  &hf_spnego_ContextFlags_mutualFlag,
  &hf_spnego_ContextFlags_replayFlag,
  &hf_spnego_ContextFlags_sequenceFlag,
  &hf_spnego_ContextFlags_anonFlag,
  &hf_spnego_ContextFlags_confFlag,
  &hf_spnego_ContextFlags_integFlag,
  NULL
};

static int
dissect_spnego_ContextFlags(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    ContextFlags_bits, 7, hf_index, ett_spnego_ContextFlags,
                                    NULL);

  return offset;
}



static int
dissect_spnego_T_mechToken(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  tvbuff_t *mechToken_tvb = NULL;

  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       &mechToken_tvb);


  /*
   * Now, we should be able to dispatch, if we've gotten a tvbuff for
   * the token and we have information on how to dissect its contents.
   */
  if (mechToken_tvb && next_level_value)
     call_dissector(next_level_value->handle, mechToken_tvb, actx->pinfo, tree);


  return offset;
}



static int
dissect_spnego_OCTET_STRING(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}


static const ber_sequence_t NegTokenInit_sequence[] = {
  { &hf_spnego_mechTypes    , BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_spnego_MechTypeList },
  { &hf_spnego_reqFlags     , BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_spnego_ContextFlags },
  { &hf_spnego_mechToken    , BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_spnego_T_mechToken },
  { &hf_spnego_mechListMIC  , BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_spnego_OCTET_STRING },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_spnego_NegTokenInit(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   NegTokenInit_sequence, hf_index, ett_spnego_NegTokenInit);

  return offset;
}



static int
dissect_spnego_T_negTokenInit(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  bool is_response = actx->pinfo->ptype == PT_TCP &&
                         actx->pinfo->srcport < 1024;

  /*
   * We decode as negTokenInit2 or negTokenInit depending on whether or not
   * we are in a response or a request. That is essentially what MS-SPNG
   * says.
   */
  if (is_response) {
    return dissect_spnego_NegTokenInit2(implicit_tag, tvb, offset,
                                        actx, tree, hf_index);
  } else {
    return dissect_spnego_NegTokenInit(implicit_tag, tvb, offset,
                                       actx, tree, hf_index);
  }


  return offset;
}


static const value_string spnego_T_negResult_vals[] = {
  {   0, "accept-completed" },
  {   1, "accept-incomplete" },
  {   2, "reject" },
  {   3, "request-mic" },
  { 0, NULL }
};


static int
dissect_spnego_T_negResult(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}



static int
dissect_spnego_T_supportedMech(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  conversation_t *conversation;

  saw_mechanism = false;

  offset = dissect_spnego_MechType(implicit_tag, tvb, offset, actx, tree, hf_index);


  /*
   * If we saw an explicit mechType we store this in the conversation so that
   * it will override any mechType we might have picked up from the
   * negTokenInit.
   */
  if(saw_mechanism){
    conversation = find_or_create_conversation(actx->pinfo);
    conversation_add_proto_data(conversation, proto_spnego, next_level_value);
  }



  return offset;
}



static int
dissect_spnego_T_responseToken(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  tvbuff_t *responseToken_tvb;


  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       &responseToken_tvb);



  /*
   * Now, we should be able to dispatch, if we've gotten a tvbuff for
   * the token and we have information on how to dissect its contents.
   * However, we should make sure that there is something in the
   * response token ...
   */
  if (responseToken_tvb && (tvb_reported_length(responseToken_tvb) > 0) ){
    gssapi_oid_value *value=next_level_value;

    if(value){
      call_dissector(value->handle, responseToken_tvb, actx->pinfo, tree);
    }
  }



  return offset;
}



static int
dissect_spnego_T_mechListMIC(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  tvbuff_t *mechListMIC_tvb;


  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       &mechListMIC_tvb);



  /*
   * Now, we should be able to dispatch, if we've gotten a tvbuff for
   * the token and we have information on how to dissect its contents.
   * However, we should make sure that there is something in the
   * response token ...
   */
  if (mechListMIC_tvb && (tvb_reported_length(mechListMIC_tvb) > 0) ){
    gssapi_oid_value *value=next_level_value;

    if(value){
      call_dissector(value->handle, mechListMIC_tvb, actx->pinfo, tree);
    }
  }



  return offset;
}


static const ber_sequence_t NegTokenTarg_sequence[] = {
  { &hf_spnego_negResult    , BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_spnego_T_negResult },
  { &hf_spnego_supportedMech, BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_spnego_T_supportedMech },
  { &hf_spnego_responseToken, BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_spnego_T_responseToken },
  { &hf_spnego_mechListMIC_01, BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_spnego_T_mechListMIC },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_spnego_NegTokenTarg(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   NegTokenTarg_sequence, hf_index, ett_spnego_NegTokenTarg);

  return offset;
}


static const ber_choice_t NegotiationToken_choice[] = {
  {   0, &hf_spnego_negTokenInit , BER_CLASS_CON, 0, 0, dissect_spnego_T_negTokenInit },
  {   1, &hf_spnego_negTokenTarg , BER_CLASS_CON, 1, 0, dissect_spnego_NegTokenTarg },
  { 0, NULL, 0, 0, 0, NULL }
};

static int
dissect_spnego_NegotiationToken(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_choice(actx, tree, tvb, offset,
                                 NegotiationToken_choice, hf_index, ett_spnego_NegotiationToken,
                                 NULL);

  return offset;
}



static int
dissect_spnego_GeneralString(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_GeneralString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}


static const ber_sequence_t NegHints_sequence[] = {
  { &hf_spnego_hintName     , BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_spnego_GeneralString },
  { &hf_spnego_hintAddress  , BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_spnego_OCTET_STRING },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_spnego_NegHints(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   NegHints_sequence, hf_index, ett_spnego_NegHints);

  return offset;
}


static const ber_sequence_t NegTokenInit2_sequence[] = {
  { &hf_spnego_mechTypes    , BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL, dissect_spnego_MechTypeList },
  { &hf_spnego_reqFlags     , BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL, dissect_spnego_ContextFlags },
  { &hf_spnego_mechToken_01 , BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_spnego_OCTET_STRING },
  { &hf_spnego_negHints     , BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_spnego_NegHints },
  { &hf_spnego_mechListMIC  , BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL, dissect_spnego_OCTET_STRING },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_spnego_NegTokenInit2(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   NegTokenInit2_sequence, hf_index, ett_spnego_NegTokenInit2);

  return offset;
}



static int
dissect_spnego_InnerContextToken(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  gssapi_oid_value *next_level_value_lcl;
  proto_item *item;
  proto_tree *subtree;
  tvbuff_t *token_tvb;
  int len;

  /*
   * XXX - what should we do if this OID doesn't match the value
   * attached to the frame or conversation?  (That would be
   * bogus, but that's not impossible - some broken implementation
   * might negotiate some security mechanism but put the OID
   * for some other security mechanism in GSS_Wrap tokens.)
   * Does it matter?
   */
  next_level_value_lcl = gssapi_lookup_oid_str(MechType_oid);

  /*
   * Now dissect the GSS_Wrap token; it's assumed to be in the
   * rest of the tvbuff.
   */
  item = proto_tree_add_item(tree, hf_spnego_wraptoken, tvb, offset, -1, ENC_NA);

  subtree = proto_item_add_subtree(item, ett_spnego_wraptoken);

  /*
   * Now, we should be able to dispatch after creating a new TVB.
   * The subdissector must return the length of the part of the
   * token it dissected, so we can return the length of the part
   * we (and it) dissected.
   */
  token_tvb = tvb_new_subset_remaining(tvb, offset);
  if (next_level_value_lcl && next_level_value_lcl->wrap_handle) {
    len = call_dissector(next_level_value_lcl->wrap_handle, token_tvb, actx->pinfo,
                         subtree);
    if (len == 0)
      offset = tvb_reported_length(tvb);
    else
      offset = offset + len;
  } else
    offset = tvb_reported_length(tvb);


  return offset;
}


static const ber_sequence_t InitialContextToken_U_sequence[] = {
  { &hf_spnego_thisMech     , BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_spnego_MechType },
  { &hf_spnego_innerContextToken, BER_CLASS_ANY, 0, BER_FLAGS_NOOWNTAG, dissect_spnego_InnerContextToken },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_spnego_InitialContextToken_U(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   InitialContextToken_U_sequence, hf_index, ett_spnego_InitialContextToken_U);

  return offset;
}



static int
dissect_spnego_InitialContextToken(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_tagged_type(implicit_tag, actx, tree, tvb, offset,
                                      hf_index, BER_CLASS_APP, 0, true, dissect_spnego_InitialContextToken_U);

  return offset;
}


static int * const HeaderFlags_bits[] = {
  &hf_spnego_HeaderFlags_unused0,
  &hf_spnego_HeaderFlags_return_dns_name,
  &hf_spnego_HeaderFlags_unused2,
  &hf_spnego_HeaderFlags_unused3,
  &hf_spnego_HeaderFlags_unused4,
  &hf_spnego_HeaderFlags_ds_12_required,
  &hf_spnego_HeaderFlags_ds_13_required,
  &hf_spnego_HeaderFlags_key_list_support_required,
  &hf_spnego_HeaderFlags_ds_10_required,
  &hf_spnego_HeaderFlags_ds_9_required,
  &hf_spnego_HeaderFlags_ds_8_required,
  &hf_spnego_HeaderFlags_web_service_required,
  &hf_spnego_HeaderFlags_ds_6_required,
  &hf_spnego_HeaderFlags_try_next_closest_site,
  &hf_spnego_HeaderFlags_is_dns_name,
  &hf_spnego_HeaderFlags_is_flat_name,
  &hf_spnego_HeaderFlags_only_ldap_needed,
  &hf_spnego_HeaderFlags_avoid_self,
  &hf_spnego_HeaderFlags_good_timeserv_pref,
  &hf_spnego_HeaderFlags_writable_required,
  &hf_spnego_HeaderFlags_timeserv_required,
  &hf_spnego_HeaderFlags_kdc_required,
  &hf_spnego_HeaderFlags_ip_required,
  &hf_spnego_HeaderFlags_background_only,
  &hf_spnego_HeaderFlags_pdc_required,
  &hf_spnego_HeaderFlags_gc_server_required,
  &hf_spnego_HeaderFlags_ds_preferred,
  &hf_spnego_HeaderFlags_ds_required,
  &hf_spnego_HeaderFlags_unused28,
  &hf_spnego_HeaderFlags_unused29,
  &hf_spnego_HeaderFlags_unused30,
  &hf_spnego_HeaderFlags_force_rediscovery,
  NULL
};

static int
dissect_spnego_HeaderFlags(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    HeaderFlags_bits, 32, hf_index, ett_spnego_HeaderFlags,
                                    NULL);

  return offset;
}



static int
dissect_spnego_T_target_realm(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {

  int8_t ber_class;
  bool pc;
  int32_t tag;

  /*
   * MIT Kerberos sends an IAKERB-HEADER like this:
   *
   * <30 2B A1 29 04 27 53 32 2D 57 32 30 31 32 2D 4C 34 2E 53 31 2D 57 32 30>
   * 0  43: SEQUENCE {
   * <A1 29 04 27 53 32 2D 57 32 30 31 32 2D 4C 34 2E 53 31 2D 57 32 30 31 32>
   * 2  41:   [1] {
   * <04 27 53 32 2D 57 32 30 31 32 2D 4C 34 2E 53 31 2D 57 32 30 31 32 2D 4C>
   * 4  39:     OCTET STRING 'S2-W2012-L4.S1-W2012-L4.W2012R2-L4.BASE'
   *    :     }
   *    :   }
   */

  get_ber_identifier(tvb, offset, &ber_class, &pc, &tag);
  if (ber_class == BER_CLASS_UNI && pc == false && tag == BER_UNI_TAG_OCTETSTRING) {
     proto_tree_add_text_internal(tree, tvb, offset, 1,
                                  "target-realm encoded as OCTET STRING: MIT Kerberos?");
     offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_OCTETSTRING,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);
  } else {
     offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_UTF8String,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);
  }


  return offset;
}


static const ber_sequence_t IAKERB_HEADER_sequence[] = {
  { &hf_spnego_target_realm , BER_CLASS_CON, 1, 0, dissect_spnego_T_target_realm },
  { &hf_spnego_cookie       , BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL, dissect_spnego_OCTET_STRING },
  { &hf_spnego_header_flags , BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL, dissect_spnego_HeaderFlags },
  { NULL, 0, 0, 0, NULL }
};

static int
dissect_spnego_IAKERB_HEADER(bool implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_sequence(implicit_tag, actx, tree, tvb, offset,
                                   IAKERB_HEADER_sequence, hf_index, ett_spnego_IAKERB_HEADER);

  return offset;
}

/*
 * This is the SPNEGO KRB5 dissector. It is not true KRB5, but some ASN.1
 * wrapped blob with an OID, USHORT token ID, and a Ticket, that is also
 * ASN.1 wrapped by the looks of it. It conforms to RFC1964.
 */

#define KRB_TOKEN_AP_REQ              0x0001
#define KRB_TOKEN_AP_REP              0x0002
#define KRB_TOKEN_AP_ERR              0x0003
#define KRB_TOKEN_GETMIC              0x0101
#define KRB_TOKEN_WRAP                0x0102
#define KRB_TOKEN_DELETE_SEC_CONTEXT  0x0201
#define KRB_TOKEN_TGT_REQ             0x0004
#define KRB_TOKEN_TGT_REP             0x0104
#define KRB_TOKEN_IAKERB_PROXY        0x0105
#define KRB_TOKEN_CFX_GETMIC          0x0404
#define KRB_TOKEN_CFX_WRAP            0x0405

static const value_string spnego_krb5_tok_id_vals[] = {
  { KRB_TOKEN_AP_REQ,             "KRB5_AP_REQ"},
  { KRB_TOKEN_AP_REP,             "KRB5_AP_REP"},
  { KRB_TOKEN_AP_ERR,             "KRB5_ERROR"},
  { KRB_TOKEN_GETMIC,             "KRB5_GSS_GetMIC" },
  { KRB_TOKEN_WRAP,               "KRB5_GSS_Wrap" },
  { KRB_TOKEN_DELETE_SEC_CONTEXT, "KRB5_GSS_Delete_sec_context" },
  { KRB_TOKEN_TGT_REQ,            "KERB_TGT_REQUEST" },
  { KRB_TOKEN_TGT_REP,            "KERB_TGT_REPLY" },
  { KRB_TOKEN_IAKERB_PROXY,       "KRB_TOKEN_IAKERB_PROXY" },
  { KRB_TOKEN_CFX_GETMIC,         "KRB_TOKEN_CFX_GetMic" },
  { KRB_TOKEN_CFX_WRAP,           "KRB_TOKEN_CFX_WRAP" },
  { 0, NULL}
};

#define KRB_SGN_ALG_DES_MAC_MD5  0x0000
#define KRB_SGN_ALG_MD2_5  0x0001
#define KRB_SGN_ALG_DES_MAC  0x0002
#define KRB_SGN_ALG_HMAC  0x0011

static const value_string spnego_krb5_sgn_alg_vals[] = {
  { KRB_SGN_ALG_DES_MAC_MD5, "DES MAC MD5"},
  { KRB_SGN_ALG_MD2_5,       "MD2.5"},
  { KRB_SGN_ALG_DES_MAC,     "DES MAC"},
  { KRB_SGN_ALG_HMAC,        "HMAC"},
  { 0, NULL}
};

#define KRB_SEAL_ALG_DES_CBC  0x0000
#define KRB_SEAL_ALG_RC4  0x0010
#define KRB_SEAL_ALG_NONE  0xffff

static const value_string spnego_krb5_seal_alg_vals[] = {
  { KRB_SEAL_ALG_DES_CBC, "DES CBC"},
  { KRB_SEAL_ALG_RC4,     "RC4"},
  { KRB_SEAL_ALG_NONE,    "None"},
  { 0, NULL}
};

/*
 * XXX - is this for SPNEGO or just GSS-API?
 * RFC 1964 is "The Kerberos Version 5 GSS-API Mechanism"; presumably one
 * can directly designate Kerberos V5 as a mechanism in GSS-API, rather
 * than designating SPNEGO as the mechanism, offering Kerberos V5, and
 * getting it accepted.
 */
static int
dissect_spnego_krb5_getmic_base(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree);
static int
dissect_spnego_krb5_wrap_base(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, uint16_t token_id, gssapi_encrypt_info_t* gssapi_encrypt);
static int
dissect_spnego_krb5_cfx_getmic_base(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree);
static int
dissect_spnego_krb5_cfx_wrap_base(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, uint16_t token_id, gssapi_encrypt_info_t* gssapi_encrypt);

static int
dissect_spnego_krb5(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
  proto_item *item;
  proto_tree *subtree;
  int offset = 0;
  uint16_t token_id;
  const char *oid;
  tvbuff_t *krb5_tvb;
  int8_t ber_class;
  bool pc, ind = 0;
  int32_t tag;
  uint32_t len;
  gssapi_encrypt_info_t* encrypt_info = (gssapi_encrypt_info_t*)data;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, true, pinfo);

  item = proto_tree_add_item(tree, hf_spnego_krb5, tvb, offset, -1, ENC_NA);

  subtree = proto_item_add_subtree(item, ett_spnego_krb5);

  /*
   * The KRB5 blob conforms to RFC1964:
   * [APPLICATION 0] {
   *   OID,
   *   USHORT (0x0001 == AP-REQ, 0x0002 == AP-REP, 0x0003 == ERROR),
   *   OCTET STRING }
   *
   * However, for some protocols, the KRB5 blob starts at the SHORT
   * and has no DER encoded header etc.
   *
   * It appears that for some other protocols the KRB5 blob is just
   * a Kerberos message, with no [APPLICATION 0] header, no OID,
   * and no USHORT.
   *
   * So:
   *
   *  If we see an [APPLICATION 0] HEADER, we show the OID and
   *  the USHORT, and then dissect the rest as a Kerberos message.
   *
   *  If we see an [APPLICATION 14] or [APPLICATION 15] header,
   *  we assume it's an AP-REQ or AP-REP message, and dissect
   *  it all as a Kerberos message.
   *
   *  Otherwise, we show the USHORT, and then dissect the rest
   *  as a Kerberos message.
   */

  /*
   * Get the first header ...
   */
  get_ber_identifier(tvb, offset, &ber_class, &pc, &tag);
  if (ber_class == BER_CLASS_APP && pc) {
    /*
     * [APPLICATION <tag>]
    */
    offset = dissect_ber_identifier(pinfo, subtree, tvb, offset, &ber_class, &pc, &tag);
    offset = dissect_ber_length(pinfo, subtree, tvb, offset, &len, &ind);

    switch (tag) {

      case 0:
        /*
         * [APPLICATION 0]
         */

        /* Next, the OID */
        offset=dissect_ber_object_identifier_str(false, &asn1_ctx, subtree, tvb, offset, hf_spnego_krb5_oid, &oid);

        token_id = tvb_get_letohs(tvb, offset);
        proto_tree_add_uint(subtree, hf_spnego_krb5_tok_id, tvb, offset, 2, token_id);

        offset += 2;

        break;

      case 14: /* [APPLICATION 14] */
      case 15: /* [APPLICATION 15] */
        /*
         * No token ID - just dissect as a Kerberos message and
         * return.
         */
        dissect_kerberos_main(tvb, pinfo, subtree, false, NULL);
        return tvb_captured_length(tvb);

      default:
        proto_tree_add_expert_format(subtree, pinfo, &ei_spnego_unknown_header, tvb, offset, 0,
          "Unknown header (class=%d, pc=%d, tag=%d)", ber_class, pc, tag);
        goto done;
      }
  } else {
      /* Next, the token ID ... */

    token_id = tvb_get_letohs(tvb, offset);
    proto_tree_add_uint(subtree, hf_spnego_krb5_tok_id, tvb, offset, 2, token_id);

    offset += 2;
  }

  switch (token_id) {

    case KRB_TOKEN_TGT_REQ:
       offset = dissect_kerberos_TGT_REQ(false, tvb, offset, &asn1_ctx, subtree, -1);
       break;
    case KRB_TOKEN_TGT_REP:
       offset = dissect_kerberos_TGT_REP(false, tvb, offset, &asn1_ctx, subtree, -1);
       break;

    case KRB_TOKEN_AP_REQ:
    case KRB_TOKEN_AP_REP:
    case KRB_TOKEN_AP_ERR:
      krb5_tvb = tvb_new_subset_remaining(tvb, offset);
      offset += dissect_kerberos_main(krb5_tvb, pinfo, subtree, false, NULL);
      break;

    case KRB_TOKEN_GETMIC:
      offset = dissect_spnego_krb5_getmic_base(tvb, offset, pinfo, subtree);
      break;

    case KRB_TOKEN_WRAP:
      offset = dissect_spnego_krb5_wrap_base(tvb, offset, pinfo, subtree, token_id, encrypt_info);
      break;

    case KRB_TOKEN_DELETE_SEC_CONTEXT:

      break;

    case KRB_TOKEN_CFX_GETMIC:
      offset = dissect_spnego_krb5_cfx_getmic_base(tvb, offset, pinfo, subtree);
      break;

    case KRB_TOKEN_CFX_WRAP:
      offset = dissect_spnego_krb5_cfx_wrap_base(tvb, offset, pinfo, subtree, token_id, encrypt_info);
      break;

    case KRB_TOKEN_IAKERB_PROXY:
      offset = dissect_spnego_IAKERB_HEADER(false, tvb, offset, &asn1_ctx, subtree, -1);
      len = tvb_captured_length_remaining(tvb, offset);
      if (len == 0) {
        break;
      }
      krb5_tvb = tvb_new_subset_remaining(tvb, offset);
      offset += dissect_kerberos_main(krb5_tvb, pinfo, subtree, false, NULL);
      break;
    default:

      break;
  }

  done:
    proto_item_set_len(item, offset);
    return tvb_captured_length(tvb);
}

#ifdef HAVE_KERBEROS
#ifndef KEYTYPE_ARCFOUR_56
# define KEYTYPE_ARCFOUR_56 24
#endif
#ifndef KEYTYPE_ARCFOUR_HMAC
# define KEYTYPE_ARCFOUR_HMAC 23
#endif
/* XXX - We should probably do a configure-time check for this instead */
#ifndef KRB5_KU_USAGE_SEAL
# define KRB5_KU_USAGE_SEAL 22
#endif

static int
arcfour_mic_key(const uint8_t *key_data, size_t key_size, int key_type,
                const uint8_t *cksum_data, size_t cksum_size,
                uint8_t *key6_data)
{
  uint8_t k5_data[HASH_MD5_LENGTH];
  uint8_t T[4] = { 0 };

  if (key_type == KEYTYPE_ARCFOUR_56) {
    uint8_t L40[14] = "fortybits";
    memcpy(L40 + 10, T, sizeof(T));
    if (ws_hmac_buffer(GCRY_MD_MD5, k5_data, L40, 14, key_data, key_size)) {
      return 0;
    }
    memset(&k5_data[7], 0xAB, 9);
  } else {
    if (ws_hmac_buffer(GCRY_MD_MD5, k5_data, T, 4, key_data, key_size)) {
      return 0;
    }
  }

  if (ws_hmac_buffer(GCRY_MD_MD5, key6_data, cksum_data, cksum_size, k5_data, HASH_MD5_LENGTH)) {
    return 0;
  }
  return 0;
}

static int
usage2arcfour(int usage)
{
  switch (usage) {
    case 3: /*KRB5_KU_AS_REP_ENC_PART 3 */
    case 9: /*KRB5_KU_TGS_REP_ENC_PART_SUB_KEY 9 */
      return 8;
    case 22: /*KRB5_KU_USAGE_SEAL 22 */
      return 13;
    case 23: /*KRB5_KU_USAGE_SIGN 23 */
        return 15;
    case 24: /*KRB5_KU_USAGE_SEQ 24 */
      return 0;
    default :
    return 0;
  }
}

static int
arcfour_mic_cksum(uint8_t *key_data, int key_length,
                  unsigned int usage,
                  uint8_t sgn_cksum[8],
                  const uint8_t *v1, size_t l1,
                  const uint8_t *v2, size_t l2,
                  const uint8_t *v3, size_t l3)
{
  static const uint8_t signature[] = "signaturekey";
  uint8_t ksign_c[HASH_MD5_LENGTH];
  uint8_t t[4];
  uint8_t digest[HASH_MD5_LENGTH];
  int rc4_usage;
  uint8_t cksum[HASH_MD5_LENGTH];
  gcry_md_hd_t md5_handle;

  rc4_usage=usage2arcfour(usage);
  if (ws_hmac_buffer(GCRY_MD_MD5, ksign_c, signature, sizeof(signature), key_data, key_length)) {
    return 0;
  }

  if (gcry_md_open(&md5_handle, GCRY_MD_MD5, 0)) {
    return 0;
  }
  t[0] = (rc4_usage >>  0) & 0xFF;
  t[1] = (rc4_usage >>  8) & 0xFF;
  t[2] = (rc4_usage >> 16) & 0xFF;
  t[3] = (rc4_usage >> 24) & 0xFF;
  gcry_md_write(md5_handle, t, 4);
  gcry_md_write(md5_handle, v1, l1);
  gcry_md_write(md5_handle, v2, l2);
  gcry_md_write(md5_handle, v3, l3);
  memcpy(digest, gcry_md_read(md5_handle, 0), HASH_MD5_LENGTH);
  gcry_md_close(md5_handle);

  if (ws_hmac_buffer(GCRY_MD_MD5, cksum, digest, HASH_MD5_LENGTH, ksign_c, HASH_MD5_LENGTH)) {
    return 0;
  }

  memcpy(sgn_cksum, cksum, 8);

  return 0;
}

/*
 * Verify padding of a gss wrapped message and return its length.
 */
static int
gssapi_verify_pad(uint8_t *wrapped_data, int wrapped_length,
                  int datalen,
                  int *padlen)
{
  uint8_t *pad;
  int padlength;
  int i;

  pad = wrapped_data + wrapped_length - 1;
  padlength = *pad;

  if (padlength > datalen)
    return 1;

  for (i = padlength; i > 0 && *pad == padlength; i--, pad--);
  if (i != 0)
    return 2;

  *padlen = padlength;

  return 0;
}

static int
decrypt_arcfour(gssapi_encrypt_info_t* gssapi_encrypt, uint8_t *input_message_buffer, uint8_t *output_message_buffer,
                uint8_t *key_value, int key_size, int key_type)
{
  uint8_t Klocaldata[16];
  int ret;
  int datalen;
  uint8_t k6_data[16];
  uint32_t SND_SEQ[2];
  uint8_t Confounder[8];
  uint8_t cksum_data[8];
  int cmp;
  int conf_flag;
  int padlen = 0;
  gcry_cipher_hd_t rc4_handle;
  int i;

  datalen = tvb_captured_length(gssapi_encrypt->gssapi_encrypted_tvb);

  if(tvb_get_ntohs(gssapi_encrypt->gssapi_wrap_tvb, 4)==0x1000){
    conf_flag=1;
  } else if (tvb_get_ntohs(gssapi_encrypt->gssapi_wrap_tvb, 4)==0xffff){
    conf_flag=0;
  } else {
    return -3;
  }

  if(tvb_get_ntohs(gssapi_encrypt->gssapi_wrap_tvb, 6)!=0xffff){
    return -4;
  }

  ret = arcfour_mic_key(key_value, key_size, key_type,
                        tvb_get_ptr(gssapi_encrypt->gssapi_wrap_tvb, 16, 8),
                        8, /* SGN_CKSUM */
                        k6_data);
  if (ret) {
    return -5;
  }

  tvb_memcpy(gssapi_encrypt->gssapi_wrap_tvb, SND_SEQ, 8, 8);
  if (gcry_cipher_open (&rc4_handle, GCRY_CIPHER_ARCFOUR, GCRY_CIPHER_MODE_STREAM, 0)) {
    return -12;
  }
  if (gcry_cipher_setkey(rc4_handle, k6_data, sizeof(k6_data))) {
    gcry_cipher_close(rc4_handle);
    return -13;
  }
  gcry_cipher_decrypt(rc4_handle, (uint8_t *)SND_SEQ, 8, NULL, 0);
  gcry_cipher_close(rc4_handle);

  memset(k6_data, 0, sizeof(k6_data));



  if (SND_SEQ[1] != 0xFFFFFFFF && SND_SEQ[1] != 0x00000000) {
    return -6;
  }


  for (i = 0; i < 16; i++)
    Klocaldata[i] = ((uint8_t *)key_value)[i] ^ 0xF0;

  ret = arcfour_mic_key(Klocaldata,sizeof(Klocaldata),key_type,
                        (const uint8_t *)SND_SEQ, 4,
                        k6_data);
  memset(Klocaldata, 0, sizeof(Klocaldata));
  if (ret) {
    return -7;
  }

  if(conf_flag) {

    tvb_memcpy(gssapi_encrypt->gssapi_wrap_tvb, Confounder, 24, 8);
    if (gcry_cipher_open (&rc4_handle, GCRY_CIPHER_ARCFOUR, GCRY_CIPHER_MODE_STREAM, 0)) {
      return -14;
    }
    if (gcry_cipher_setkey(rc4_handle, k6_data, sizeof(k6_data))) {
      gcry_cipher_close(rc4_handle);
      return -15;
    }

    gcry_cipher_decrypt(rc4_handle, Confounder, 8, NULL, 0);
    gcry_cipher_decrypt(rc4_handle, output_message_buffer, datalen, input_message_buffer, datalen);
    gcry_cipher_close(rc4_handle);
  } else {
    tvb_memcpy(gssapi_encrypt->gssapi_wrap_tvb, Confounder, 24, 8);
    memcpy(output_message_buffer, input_message_buffer, datalen);
  }
  memset(k6_data, 0, sizeof(k6_data));

  /* only normal (i.e. non DCE style  wrapping use padding ? */
  if(gssapi_encrypt->decrypt_gssapi_tvb==DECRYPT_GSSAPI_NORMAL){
    ret = gssapi_verify_pad(output_message_buffer,datalen,datalen, &padlen);
    if (ret) {
      return -9;
    }
    datalen -= padlen;
  }

  /* don't know what the checksum looks like for dce style gssapi */
  if(gssapi_encrypt->decrypt_gssapi_tvb==DECRYPT_GSSAPI_NORMAL){
    ret = arcfour_mic_cksum(key_value, key_size, KRB5_KU_USAGE_SEAL,
                            cksum_data,
                            tvb_get_ptr(gssapi_encrypt->gssapi_wrap_tvb, 0, 8), 8,
                            Confounder, sizeof(Confounder), output_message_buffer,
                            datalen + padlen);
    if (ret) {
      return -10;
    }

    cmp = tvb_memeql(gssapi_encrypt->gssapi_wrap_tvb, 16, cksum_data, 8); /* SGN_CKSUM */
    if (cmp) {
      return -11;
    }
  }

  return datalen;
}



#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)

static void
decrypt_gssapi_krb_arcfour_wrap(proto_tree *tree _U_, packet_info *pinfo, tvbuff_t *tvb, int keytype, gssapi_encrypt_info_t* gssapi_encrypt)
{
  int ret;
  enc_key_t *ek;
  int length;
  const uint8_t *original_data;

  uint8_t *cryptocopy=NULL; /* workaround for pre-0.6.1 heimdal bug */
  uint8_t *output_message_buffer;

  length=tvb_captured_length(gssapi_encrypt->gssapi_encrypted_tvb);
  original_data=tvb_get_ptr(gssapi_encrypt->gssapi_encrypted_tvb, 0, length);

  /* don't do anything if we are not attempting to decrypt data */
/*
  if(!krb_decrypt){
    return;
  }
*/
  /* XXX we should only do this for first time, then store somewhere */
  /* XXX We also need to re-read the keytab when the preference changes */

  cryptocopy=(uint8_t *)wmem_alloc(pinfo->pool, length);
  output_message_buffer=(uint8_t *)wmem_alloc(pinfo->pool, length);

  for(ek=enc_key_list;ek;ek=ek->next){
    /* shortcircuit and bail out if enctypes are not matching */
    if(ek->keytype!=keytype){
      continue;
    }

    /* pre-0.6.1 versions of Heimdal would sometimes change
      the cryptotext data even when the decryption failed.
      This would obviously not work since we iterate over the
      keys. So just give it a copy of the crypto data instead.
      This has been seen for RC4-HMAC blobs.
    */
    memcpy(cryptocopy, original_data, length);
    ret=decrypt_arcfour(gssapi_encrypt,
                        cryptocopy,
                        output_message_buffer,
                        ek->keyvalue,
                        ek->keylength,
                        ek->keytype);
    if (ret >= 0) {
      expert_add_info_format(pinfo, NULL, &ei_spnego_decrypted_keytype,
                             "Decrypted keytype %d in frame %u using %s",
                             ek->keytype, pinfo->num, ek->key_origin);

      gssapi_encrypt->gssapi_decrypted_tvb=tvb_new_child_real_data(tvb, output_message_buffer, ret, ret);
      add_new_data_source(pinfo, gssapi_encrypt->gssapi_decrypted_tvb, "Decrypted GSS-Krb5");
      return;
    }
  }
}

/* borrowed from heimdal */
static int
rrc_rotate(uint8_t *data, int len, uint16_t rrc, int unrotate)
{
  uint8_t *tmp, buf[256];
  size_t left;

  if (len == 0)
    return 0;

  rrc %= len;

  if (rrc == 0)
    return 0;

  left = len - rrc;

  if (rrc <= sizeof(buf)) {
    tmp = buf;
  } else {
    tmp = (uint8_t *)g_malloc(rrc);
    if (tmp == NULL)
      return -1;
  }

  if (unrotate) {
    memcpy(tmp, data, rrc);
    memmove(data, data + rrc, left);
    memcpy(data + left, tmp, rrc);
  } else {
    memcpy(tmp, data + left, rrc);
    memmove(data + rrc, data, left);
    memcpy(data, tmp, rrc);
  }

  if (rrc > sizeof(buf))
    g_free(tmp);

  return 0;
}


static void
decrypt_gssapi_krb_cfx_wrap(proto_tree *tree,
                            packet_info *pinfo,
                            tvbuff_t *checksum_tvb,
                            gssapi_encrypt_info_t* gssapi_encrypt,
                            uint16_t ec _U_,
                            uint16_t rrc,
                            int keytype,
                            unsigned int usage)
{
  uint8_t *rotated;
  uint8_t *output;
  int datalen;
  tvbuff_t *next_tvb;

  /* don't do anything if we are not attempting to decrypt data */
  if(!krb_decrypt){
    return;
  }

  if (gssapi_encrypt->decrypt_gssapi_tvb==DECRYPT_GSSAPI_DCE) {
    tvbuff_t *out_tvb = NULL;

    out_tvb = decrypt_krb5_krb_cfx_dce(tree, pinfo, usage, keytype,
                                       gssapi_encrypt->gssapi_header_tvb,
                                       gssapi_encrypt->gssapi_encrypted_tvb,
                                       gssapi_encrypt->gssapi_trailer_tvb,
                                       checksum_tvb);
    if (out_tvb) {
      gssapi_encrypt->gssapi_decrypted_tvb = out_tvb;
      add_new_data_source(pinfo, gssapi_encrypt->gssapi_decrypted_tvb, "Decrypted GSS-Krb5 CFX DCE");
    }
    return;
  }

  datalen = tvb_captured_length(checksum_tvb) + tvb_captured_length(gssapi_encrypt->gssapi_encrypted_tvb);

  rotated = (uint8_t *)wmem_alloc(pinfo->pool, datalen);

  tvb_memcpy(checksum_tvb, rotated, 0, tvb_captured_length(checksum_tvb));
  tvb_memcpy(gssapi_encrypt->gssapi_encrypted_tvb, rotated + tvb_captured_length(checksum_tvb),
             0, tvb_captured_length(gssapi_encrypt->gssapi_encrypted_tvb));

  rrc_rotate(rotated, datalen, rrc, true);

  next_tvb=tvb_new_child_real_data(gssapi_encrypt->gssapi_encrypted_tvb, rotated,
                                   datalen, datalen);
  add_new_data_source(pinfo, next_tvb, "GSSAPI CFX");

  output = decrypt_krb5_data(tree, pinfo, usage, next_tvb, keytype, &datalen);

  if (output) {
    uint8_t *outdata;

    outdata = (uint8_t *)wmem_memdup(pinfo->pool, output, tvb_captured_length(gssapi_encrypt->gssapi_encrypted_tvb));

    gssapi_encrypt->gssapi_decrypted_tvb=tvb_new_child_real_data(gssapi_encrypt->gssapi_encrypted_tvb,
      outdata,
      tvb_captured_length(gssapi_encrypt->gssapi_encrypted_tvb),
      tvb_captured_length(gssapi_encrypt->gssapi_encrypted_tvb));
    add_new_data_source(pinfo, gssapi_encrypt->gssapi_decrypted_tvb, "Decrypted GSS-Krb5");
  }
}

#endif /* HAVE_HEIMDAL_KERBEROS || HAVE_MIT_KERBEROS */


#endif

/*
 * This is for GSSAPI Wrap tokens ...
 */
static int
dissect_spnego_krb5_wrap_base(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, uint16_t token_id, gssapi_encrypt_info_t* gssapi_encrypt)
{
  uint16_t sgn_alg, seal_alg;
#ifdef HAVE_KERBEROS
  int start_offset=offset;
#else
  (void) pinfo;
  (void) token_id;
#endif

  /*
   * The KRB5 blob conforms to RFC1964:
   *   USHORT (0x0102 == GSS_Wrap)
   *   and so on }
   */

  /* Now, the sign and seal algorithms ... */

  sgn_alg = tvb_get_letohs(tvb, offset);
  proto_tree_add_uint(tree, hf_spnego_krb5_sgn_alg, tvb, offset, 2, sgn_alg);

  offset += 2;

  seal_alg = tvb_get_letohs(tvb, offset);
  proto_tree_add_uint(tree, hf_spnego_krb5_seal_alg, tvb, offset, 2, seal_alg);

  offset += 2;

  /* Skip the filler */

  offset += 2;

  /* Encrypted sequence number */

  proto_tree_add_item(tree, hf_spnego_krb5_snd_seq, tvb, offset, 8, ENC_NA);

  offset += 8;

  /* Checksum of plaintext padded data */

  proto_tree_add_item(tree, hf_spnego_krb5_sgn_cksum, tvb, offset, 8, ENC_NA);

  offset += 8;

  /*
   * At least according to draft-brezak-win2k-krb-rc4-hmac-04,
   * if the signing algorithm is KRB_SGN_ALG_HMAC, there's an
   * extra 8 bytes of "Random confounder" after the checksum.
   * It certainly confounds code expecting all Kerberos 5
   * GSS_Wrap() tokens to look the same....
   */
  if ((sgn_alg == KRB_SGN_ALG_HMAC) ||
      /* there also seems to be a confounder for DES MAC MD5 - certainly seen when using with
         SASL with LDAP between a Java client and Active Directory. If this breaks other things
         we may need to make this an option. gal 17/2/06 */
      (sgn_alg == KRB_SGN_ALG_DES_MAC_MD5)) {
    proto_tree_add_item(tree, hf_spnego_krb5_confounder, tvb, offset, 8, ENC_NA);
    offset += 8;
  }

  /* Is the data encrypted? */
  if (gssapi_encrypt != NULL)
    gssapi_encrypt->gssapi_data_encrypted=(seal_alg!=KRB_SEAL_ALG_NONE);

#ifdef HAVE_KERBEROS
#define GSS_ARCFOUR_WRAP_TOKEN_SIZE 32
  if(gssapi_encrypt && gssapi_encrypt->decrypt_gssapi_tvb){
    /* if the caller did not provide a tvb, then we just use
       whatever is left of our current tvb.
    */
    if(!gssapi_encrypt->gssapi_encrypted_tvb){
      int len;
      len=tvb_reported_length_remaining(tvb,offset);
      if(len>tvb_captured_length_remaining(tvb, offset)){
        /* no point in trying to decrypt,
           we don't have the full pdu.
        */
        return offset;
      }
      gssapi_encrypt->gssapi_encrypted_tvb = tvb_new_subset_length(
          tvb, offset, len);
    }

    /* if this is KRB5 wrapped rc4-hmac */
    if((token_id==KRB_TOKEN_WRAP)
     &&(sgn_alg==KRB_SGN_ALG_HMAC)
     &&(seal_alg==KRB_SEAL_ALG_RC4)){
      /* do we need to create a tvb for the wrapper
         as well ?
      */
      if(!gssapi_encrypt->gssapi_wrap_tvb){
        gssapi_encrypt->gssapi_wrap_tvb = tvb_new_subset_length(
          tvb, start_offset-2,
          GSS_ARCFOUR_WRAP_TOKEN_SIZE);
      }
#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)
      decrypt_gssapi_krb_arcfour_wrap(tree,
        pinfo,
        tvb,
        KEYTYPE_ARCFOUR_HMAC,
        gssapi_encrypt);
#endif /* HAVE_HEIMDAL_KERBEROS || HAVE_MIT_KERBEROS */
    }
  }
#endif
  /*
   * Return the offset past the checksum, so that we know where
   * the data we're wrapped around starts.  Also, set the length
   * of our top-level item to that offset, so it doesn't cover
   * the data we're wrapped around.
   *
   * Note that for DCERPC the GSSAPI blobs comes after the data it wraps,
   * not before.
   */
  return offset;
}

/*
 * XXX - This is for GSSAPI GetMIC tokens ...
 */
static int
dissect_spnego_krb5_getmic_base(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
  uint16_t sgn_alg;

  /*
   * The KRB5 blob conforms to RFC1964:
   *   USHORT (0x0101 == GSS_GetMIC)
   *   and so on }
   */

  /* Now, the sign algorithm ... */

  sgn_alg = tvb_get_letohs(tvb, offset);
  proto_tree_add_uint(tree, hf_spnego_krb5_sgn_alg, tvb, offset, 2, sgn_alg);

  offset += 2;

  /* Skip the filler */

  offset += 4;

  /* Encrypted sequence number */

  proto_tree_add_item(tree, hf_spnego_krb5_snd_seq, tvb, offset, 8, ENC_NA);

  offset += 8;

  /* Checksum of plaintext padded data */

  proto_tree_add_item(tree, hf_spnego_krb5_sgn_cksum, tvb, offset, 8, ENC_NA);

  offset += 8;

  /*
   * At least according to draft-brezak-win2k-krb-rc4-hmac-04,
   * if the signing algorithm is KRB_SGN_ALG_HMAC, there's an
   * extra 8 bytes of "Random confounder" after the checksum.
   * It certainly confounds code expecting all Kerberos 5
   * GSS_Wrap() tokens to look the same....
   *
   * The exception is DNS/TSIG where there is no such confounder
   * so we need to test here if there are more bytes in our tvb or not.
   *  -- ronnie
   */
  if (tvb_reported_length_remaining(tvb, offset)) {
    if (sgn_alg == KRB_SGN_ALG_HMAC) {
      proto_tree_add_item(tree, hf_spnego_krb5_confounder, tvb, offset, 8, ENC_NA);

      offset += 8;
    }
  }

  /*
   * Return the offset past the checksum, so that we know where
   * the data we're wrapped around starts.  Also, set the length
   * of our top-level item to that offset, so it doesn't cover
   * the data we're wrapped around.
   */

  return offset;
}

static int
dissect_spnego_krb5_cfx_flags(tvbuff_t *tvb, int offset,
                              proto_tree *spnego_krb5_tree,
                              uint8_t cfx_flags _U_)
{
  static int * const flags[] = {
    &hf_spnego_krb5_cfx_flags_04,
    &hf_spnego_krb5_cfx_flags_02,
    &hf_spnego_krb5_cfx_flags_01,
    NULL
  };

  proto_tree_add_bitmask(spnego_krb5_tree, tvb, offset, hf_spnego_krb5_cfx_flags, ett_spnego_krb5_cfx_flags, flags, ENC_NA);
  return (offset + 1);
}

/*
 * This is for GSSAPI CFX Wrap tokens ...
 */
static int
dissect_spnego_krb5_cfx_wrap_base(tvbuff_t *tvb, int offset, packet_info *pinfo, proto_tree *tree, uint16_t token_id _U_, gssapi_encrypt_info_t* gssapi_encrypt)
{
  uint8_t flags;
  uint16_t ec;
#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)
  uint16_t rrc;
#else
  (void) pinfo;
#endif
  int checksum_size;
  int start_offset=offset;

  /*
   * The KRB5 blob conforms to RFC4121:
   *   USHORT (0x0504)
   *   and so on }
   */

  /* Now, the sign and seal algorithms ... */

  flags = tvb_get_uint8(tvb, offset);
  offset = dissect_spnego_krb5_cfx_flags(tvb, offset, tree, flags);

  if (gssapi_encrypt != NULL)
    gssapi_encrypt->gssapi_data_encrypted=(flags & 2);

  /* Skip the filler */

  proto_tree_add_item(tree, hf_spnego_krb5_filler, tvb, offset, 1, ENC_NA);
  offset += 1;

  /* EC */
  ec = tvb_get_ntohs(tvb, offset);
  proto_tree_add_item(tree, hf_spnego_krb5_cfx_ec, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  /* RRC */
#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)
  rrc = tvb_get_ntohs(tvb, offset);
#endif
  proto_tree_add_item(tree, hf_spnego_krb5_cfx_rrc, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  /* sequence number */

  proto_tree_add_item(tree, hf_spnego_krb5_cfx_seq, tvb, offset, 8, ENC_BIG_ENDIAN);
  offset += 8;

  if (gssapi_encrypt == NULL) /* Probably shouldn't happen, but just protect ourselves */
    return offset;

  /* Checksum of plaintext padded data */

  if (gssapi_encrypt->gssapi_data_encrypted) {
    checksum_size = 44 + ec;

    proto_tree_add_item(tree, hf_spnego_krb5_sgn_cksum, tvb, offset, checksum_size, ENC_NA);
    offset += checksum_size;

  } else {
    int returned_offset;
    int inner_token_len = 0;

    /*
     * We know we have a wrap token, but we have to let the proto
     * above us decode that, so hand it back in gssapi_wrap_tvb
     * and put the checksum in the tree.
     */

    checksum_size = ec;

    inner_token_len = tvb_reported_length_remaining(tvb, offset);
    if (inner_token_len > ec) {
      inner_token_len -= ec;
    }

    /*
     * We handle only the two common cases for now
     * (rrc == 0 and rrc == ec)
     */
#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)
    if (rrc == ec) {
      proto_tree_add_item(tree, hf_spnego_krb5_sgn_cksum, tvb, offset, checksum_size, ENC_NA);
      offset += checksum_size;
    }
#endif

    returned_offset = offset;
    gssapi_encrypt->gssapi_wrap_tvb = tvb_new_subset_length(tvb, offset,
            inner_token_len);
    gssapi_encrypt->gssapi_decrypted_tvb = tvb_new_subset_length(tvb, offset,
            inner_token_len);

    offset += inner_token_len;

#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)
    if (rrc == 0)
#endif
    {
      proto_tree_add_item(tree, hf_spnego_krb5_sgn_cksum, tvb, offset, checksum_size, ENC_NA);
    }

    /*
     * Return an offset that puts our caller before the inner
     * token. This is better than before, but we still see the
     * checksum included in the LDAP query at times.
     */
    return returned_offset;
  }

  if(gssapi_encrypt->decrypt_gssapi_tvb){
    /* if the caller did not provide a tvb, then we just use
       whatever is left of our current tvb.
    */
    if(!gssapi_encrypt->gssapi_encrypted_tvb){
      int len;
      len=tvb_reported_length_remaining(tvb,offset);
      if(len>tvb_captured_length_remaining(tvb, offset)){
        /* no point in trying to decrypt,
           we don't have the full pdu.
        */
        return offset;
      }
      gssapi_encrypt->gssapi_encrypted_tvb = tvb_new_subset_length_caplen(
          tvb, offset, len, len);
    }

    if (gssapi_encrypt->gssapi_data_encrypted) {
      /* do we need to create a tvb for the wrapper
         as well ?
      */
      if(!gssapi_encrypt->gssapi_wrap_tvb){
        gssapi_encrypt->gssapi_wrap_tvb = tvb_new_subset_length(
          tvb, start_offset-2,
          offset - (start_offset-2));
      }
    }
  }

#if defined(HAVE_HEIMDAL_KERBEROS) || defined(HAVE_MIT_KERBEROS)
{
  tvbuff_t *checksum_tvb = tvb_new_subset_length(tvb, 16, checksum_size);

  if (gssapi_encrypt->gssapi_data_encrypted) {
    if(gssapi_encrypt->gssapi_encrypted_tvb){
      decrypt_gssapi_krb_cfx_wrap(tree,
        pinfo,
        checksum_tvb,
        gssapi_encrypt,
        ec,
        rrc,
        -1,
        (flags & 0x0001)?
        KRB5_KU_USAGE_ACCEPTOR_SEAL:
        KRB5_KU_USAGE_INITIATOR_SEAL);
    }
  }
}
#endif /* HAVE_HEIMDAL_KERBEROS || HAVE_MIT_KERBEROS */

  /*
   * Return the offset past the checksum, so that we know where
   * the data we're wrapped around starts.  Also, set the length
   * of our top-level item to that offset, so it doesn't cover
   * the data we're wrapped around.
   *
   * Note that for DCERPC the GSSAPI blobs comes after the data it wraps,
   * not before.
   */
  return offset;
}

/*
 * XXX - This is for GSSAPI CFX GetMIC tokens ...
 */
static int
dissect_spnego_krb5_cfx_getmic_base(tvbuff_t *tvb, int offset, packet_info *pinfo _U_, proto_tree *tree)
{
  uint8_t flags;
  int checksum_size;

  /*
   * The KRB5 blob conforms to RFC4121:
   *   USHORT (0x0404 == GSS_GetMIC)
   *   and so on }
   */

  flags = tvb_get_uint8(tvb, offset);
  offset = dissect_spnego_krb5_cfx_flags(tvb, offset, tree, flags);

  /* Skip the filler */

  proto_tree_add_item(tree, hf_spnego_krb5_filler, tvb, offset, 5, ENC_NA);
  offset += 5;

  /* sequence number */

  proto_tree_add_item(tree, hf_spnego_krb5_cfx_seq, tvb, offset, 8, ENC_BIG_ENDIAN);
  offset += 8;

  /* Checksum of plaintext padded data */

  checksum_size = tvb_captured_length_remaining(tvb, offset);

  proto_tree_add_item(tree, hf_spnego_krb5_sgn_cksum, tvb, offset,  checksum_size, ENC_NA);
  offset += checksum_size;

  /*
   * Return the offset past the checksum, so that we know where
   * the data we're wrapped around starts.  Also, set the length
   * of our top-level item to that offset, so it doesn't cover
   * the data we're wrapped around.
   */

  return offset;
}

/*
 * XXX - is this for SPNEGO or just GSS-API?
 * RFC 1964 is "The Kerberos Version 5 GSS-API Mechanism"; presumably one
 * can directly designate Kerberos V5 as a mechanism in GSS-API, rather
 * than designating SPNEGO as the mechanism, offering Kerberos V5, and
 * getting it accepted.
 */
static int
dissect_spnego_krb5_wrap(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data)
{
  proto_item *item;
  proto_tree *subtree;
  int offset = 0;
  uint16_t token_id;
  gssapi_encrypt_info_t* encrypt_info = (gssapi_encrypt_info_t*)data;

  item = proto_tree_add_item(tree, hf_spnego_krb5, tvb, 0, -1, ENC_NA);

  subtree = proto_item_add_subtree(item, ett_spnego_krb5);

  /*
   * The KRB5 blob conforms to RFC1964:
   *   USHORT (0x0102 == GSS_Wrap)
   *   and so on }
   */

  /* First, the token ID ... */

  token_id = tvb_get_letohs(tvb, offset);
  proto_tree_add_uint(subtree, hf_spnego_krb5_tok_id, tvb, offset, 2, token_id);

  offset += 2;

  switch (token_id) {
  case KRB_TOKEN_GETMIC:
    offset = dissect_spnego_krb5_getmic_base(tvb, offset, pinfo, subtree);
    break;

  case KRB_TOKEN_WRAP:
    offset = dissect_spnego_krb5_wrap_base(tvb, offset, pinfo, subtree, token_id, encrypt_info);
    break;

  case KRB_TOKEN_CFX_GETMIC:
    offset = dissect_spnego_krb5_cfx_getmic_base(tvb, offset, pinfo, subtree);
    break;

  case KRB_TOKEN_CFX_WRAP:
    offset = dissect_spnego_krb5_cfx_wrap_base(tvb, offset, pinfo, subtree, token_id, encrypt_info);
    break;

  default:

    break;
  }

  /*
   * Return the offset past the checksum, so that we know where
   * the data we're wrapped around starts.  Also, set the length
   * of our top-level item to that offset, so it doesn't cover
   * the data we're wrapped around.
   */
  proto_item_set_len(item, offset);
  return offset;
}

/* Spnego stuff from here */

static int
dissect_spnego_wrap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  proto_item *item;
  proto_tree *subtree;
  int offset = 0;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, true, pinfo);

  MechType_oid = NULL;

  /*
   * We need this later, so lets get it now ...
   * It has to be per-frame as there can be more than one GSS-API
   * negotiation in a conversation.
   */


  item = proto_tree_add_item(tree, proto_spnego, tvb, offset, -1, ENC_NA);

  subtree = proto_item_add_subtree(item, ett_spnego);
  /*
   * The TVB contains a [0] header and a sequence that consists of an
   * object ID and a blob containing the data ...
   * XXX - is this RFC 2743's "Mechanism-Independent Token Format",
   * with the "optional" "use in non-initial tokens" being chosen.
   * ASN1 code addet to spnego.asn to handle this.
   */

  offset = dissect_spnego_InitialContextToken(false, tvb, offset, &asn1_ctx , subtree, -1);

  return offset;
}


static int
dissect_spnego(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree, void* data _U_)
{
  proto_item *item;
  proto_tree *subtree;
  int offset = 0;
  conversation_t *conversation;
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, true, pinfo);

  /*
   * We need this later, so lets get it now ...
   * It has to be per-frame as there can be more than one GSS-API
   * negotiation in a conversation.
   */
  next_level_value = (gssapi_oid_value *)p_get_proto_data(wmem_file_scope(), pinfo, proto_spnego, 0);
  if (!next_level_value && !pinfo->fd->visited) {
      /*
       * No handle attached to this frame, but it's the first
       * pass, so it'd be attached to the conversation.
       * If we have a conversation, try to get the handle,
       * and if we get one, attach it to the frame.
       */
      conversation = find_conversation_pinfo(pinfo, 0);

      if (conversation) {
        next_level_value = (gssapi_oid_value *)conversation_get_proto_data(conversation, proto_spnego);
        if (next_level_value)
          p_add_proto_data(wmem_file_scope(), pinfo, proto_spnego, 0, next_level_value);
      }
  }

  item = proto_tree_add_item(parent_tree, proto_spnego, tvb, offset, -1, ENC_NA);

  subtree = proto_item_add_subtree(item, ett_spnego);

  /*
   * The TVB contains a [0] header and a sequence that consists of an
   * object ID and a blob containing the data ...
   * Actually, it contains, according to RFC2478:
   * NegotiationToken ::= CHOICE {
   *          negTokenInit [0] NegTokenInit,
   *          negTokenTarg [1] NegTokenTarg }
   * NegTokenInit ::= SEQUENCE {
   *          mechTypes [0] MechTypeList OPTIONAL,
   *          reqFlags [1] ContextFlags OPTIONAL,
   *          mechToken [2] OCTET STRING OPTIONAL,
   *          mechListMIC [3] OCTET STRING OPTIONAL }
   * NegTokenTarg ::= SEQUENCE {
   *          negResult [0] ENUMERATED {
   *              accept_completed (0),
   *              accept_incomplete (1),
   *              reject (2) } OPTIONAL,
   *          supportedMech [1] MechType OPTIONAL,
   *          responseToken [2] OCTET STRING OPTIONAL,
   *          mechListMIC [3] OCTET STRING OPTIONAL }
   *
   * Windows typically includes mechTypes and mechListMic ('NONE'
   * in the case of NTLMSSP only).
   * It seems to duplicate the responseToken into the mechListMic field
   * as well. Naughty, naughty.
   *
   */
  dissect_spnego_NegotiationToken(false, tvb, offset, &asn1_ctx, subtree, -1);
  return tvb_captured_length(tvb);
}

/*--- proto_register_spnego -------------------------------------------*/
void proto_register_spnego(void) {

  /* List of fields */
  static hf_register_info hf[] = {
    { &hf_spnego_wraptoken,
      { "wrapToken", "spnego.wraptoken",
        FT_NONE, BASE_NONE, NULL, 0x0, "SPNEGO wrapToken",
        HFILL}},
    { &hf_spnego_krb5,
      { "krb5_blob", "spnego.krb5.blob", FT_BYTES,
        BASE_NONE, NULL, 0, NULL, HFILL }},
    { &hf_spnego_krb5_oid,
      { "KRB5 OID", "spnego.krb5_oid", FT_STRING,
        BASE_NONE, NULL, 0, NULL, HFILL }},
    { &hf_spnego_krb5_tok_id,
      { "krb5_tok_id", "spnego.krb5.tok_id", FT_UINT16, BASE_HEX,
        VALS(spnego_krb5_tok_id_vals), 0, "KRB5 Token Id", HFILL}},
    { &hf_spnego_krb5_sgn_alg,
      { "krb5_sgn_alg", "spnego.krb5.sgn_alg", FT_UINT16, BASE_HEX,
        VALS(spnego_krb5_sgn_alg_vals), 0, "KRB5 Signing Algorithm", HFILL}},
    { &hf_spnego_krb5_seal_alg,
      { "krb5_seal_alg", "spnego.krb5.seal_alg", FT_UINT16, BASE_HEX,
        VALS(spnego_krb5_seal_alg_vals), 0, "KRB5 Sealing Algorithm", HFILL}},
    { &hf_spnego_krb5_snd_seq,
      { "krb5_snd_seq", "spnego.krb5.snd_seq", FT_BYTES, BASE_NONE,
        NULL, 0, "KRB5 Encrypted Sequence Number", HFILL}},
    { &hf_spnego_krb5_sgn_cksum,
      { "krb5_sgn_cksum", "spnego.krb5.sgn_cksum", FT_BYTES, BASE_NONE,
        NULL, 0, "KRB5 Data Checksum", HFILL}},
    { &hf_spnego_krb5_confounder,
      { "krb5_confounder", "spnego.krb5.confounder", FT_BYTES, BASE_NONE,
        NULL, 0, "KRB5 Confounder", HFILL}},
    { &hf_spnego_krb5_filler,
      { "krb5_filler", "spnego.krb5.filler", FT_BYTES, BASE_NONE,
        NULL, 0, "KRB5 Filler", HFILL}},
    { &hf_spnego_krb5_cfx_flags,
      { "krb5_cfx_flags", "spnego.krb5.cfx_flags", FT_UINT8, BASE_HEX,
        NULL, 0, "KRB5 CFX Flags", HFILL}},
    { &hf_spnego_krb5_cfx_flags_01,
      { "SendByAcceptor", "spnego.krb5.send_by_acceptor", FT_BOOLEAN, 8,
        TFS (&tfs_set_notset), 0x01, NULL, HFILL}},
    { &hf_spnego_krb5_cfx_flags_02,
      { "Sealed", "spnego.krb5.sealed", FT_BOOLEAN, 8,
        TFS (&tfs_set_notset), 0x02, NULL, HFILL}},
    { &hf_spnego_krb5_cfx_flags_04,
      { "AcceptorSubkey", "spnego.krb5.acceptor_subkey", FT_BOOLEAN, 8,
        TFS (&tfs_set_notset), 0x04, NULL, HFILL}},
    { &hf_spnego_krb5_cfx_ec,
      { "krb5_cfx_ec", "spnego.krb5.cfx_ec", FT_UINT16, BASE_DEC,
        NULL, 0, "KRB5 CFX Extra Count", HFILL}},
    { &hf_spnego_krb5_cfx_rrc,
      { "krb5_cfx_rrc", "spnego.krb5.cfx_rrc", FT_UINT16, BASE_DEC,
        NULL, 0, "KRB5 CFX Right Rotation Count", HFILL}},
    { &hf_spnego_krb5_cfx_seq,
      { "krb5_cfx_seq", "spnego.krb5.cfx_seq", FT_UINT64, BASE_DEC,
        NULL, 0, "KRB5 Sequence Number", HFILL}},

    { &hf_spnego_negTokenInit,
      { "negTokenInit", "spnego.negTokenInit_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_negTokenTarg,
      { "negTokenTarg", "spnego.negTokenTarg_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_MechTypeList_item,
      { "MechType", "spnego.MechType",
        FT_OID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_mechTypes,
      { "mechTypes", "spnego.mechTypes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "MechTypeList", HFILL }},
    { &hf_spnego_reqFlags,
      { "reqFlags", "spnego.reqFlags",
        FT_BYTES, BASE_NONE, NULL, 0,
        "ContextFlags", HFILL }},
    { &hf_spnego_mechToken,
      { "mechToken", "spnego.mechToken",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_mechListMIC,
      { "mechListMIC", "spnego.mechListMIC",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_spnego_hintName,
      { "hintName", "spnego.hintName",
        FT_STRING, BASE_NONE, NULL, 0,
        "GeneralString", HFILL }},
    { &hf_spnego_hintAddress,
      { "hintAddress", "spnego.hintAddress",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_spnego_mechToken_01,
      { "mechToken", "spnego.mechToken",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_spnego_negHints,
      { "negHints", "spnego.negHints_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_negResult,
      { "negResult", "spnego.negResult",
        FT_UINT32, BASE_DEC, VALS(spnego_T_negResult_vals), 0,
        NULL, HFILL }},
    { &hf_spnego_supportedMech,
      { "supportedMech", "spnego.supportedMech",
        FT_OID, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_responseToken,
      { "responseToken", "spnego.responseToken",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_mechListMIC_01,
      { "mechListMIC", "spnego.mechListMIC",
        FT_BYTES, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_thisMech,
      { "thisMech", "spnego.thisMech",
        FT_OID, BASE_NONE, NULL, 0,
        "MechType", HFILL }},
    { &hf_spnego_innerContextToken,
      { "innerContextToken", "spnego.innerContextToken_element",
        FT_NONE, BASE_NONE, NULL, 0,
        NULL, HFILL }},
    { &hf_spnego_target_realm,
      { "target-realm", "spnego.target_realm",
        FT_STRING, BASE_NONE, NULL, 0,
        "T_target_realm", HFILL }},
    { &hf_spnego_cookie,
      { "cookie", "spnego.cookie",
        FT_BYTES, BASE_NONE, NULL, 0,
        "OCTET_STRING", HFILL }},
    { &hf_spnego_header_flags,
      { "header-flags", "spnego.header_flags",
        FT_BYTES, BASE_NONE, NULL, 0,
        "HeaderFlags", HFILL }},
    { &hf_spnego_ContextFlags_delegFlag,
      { "delegFlag", "spnego.ContextFlags.delegFlag",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_spnego_ContextFlags_mutualFlag,
      { "mutualFlag", "spnego.ContextFlags.mutualFlag",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_spnego_ContextFlags_replayFlag,
      { "replayFlag", "spnego.ContextFlags.replayFlag",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_spnego_ContextFlags_sequenceFlag,
      { "sequenceFlag", "spnego.ContextFlags.sequenceFlag",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_spnego_ContextFlags_anonFlag,
      { "anonFlag", "spnego.ContextFlags.anonFlag",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_spnego_ContextFlags_confFlag,
      { "confFlag", "spnego.ContextFlags.confFlag",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_spnego_ContextFlags_integFlag,
      { "integFlag", "spnego.ContextFlags.integFlag",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_unused0,
      { "unused0", "spnego.HeaderFlags.unused0",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_return_dns_name,
      { "return-dns-name", "spnego.HeaderFlags.return.dns.name",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_unused2,
      { "unused2", "spnego.HeaderFlags.unused2",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_unused3,
      { "unused3", "spnego.HeaderFlags.unused3",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_unused4,
      { "unused4", "spnego.HeaderFlags.unused4",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_12_required,
      { "ds-12-required", "spnego.HeaderFlags.ds.12.required",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_13_required,
      { "ds-13-required", "spnego.HeaderFlags.ds.13.required",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_key_list_support_required,
      { "key-list-support-required", "spnego.HeaderFlags.key.list.support.required",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_10_required,
      { "ds-10-required", "spnego.HeaderFlags.ds.10.required",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_9_required,
      { "ds-9-required", "spnego.HeaderFlags.ds.9.required",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_8_required,
      { "ds-8-required", "spnego.HeaderFlags.ds.8.required",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_web_service_required,
      { "web-service-required", "spnego.HeaderFlags.web.service.required",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_6_required,
      { "ds-6-required", "spnego.HeaderFlags.ds.6.required",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_try_next_closest_site,
      { "try-next-closest-site", "spnego.HeaderFlags.try.next.closest.site",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_is_dns_name,
      { "is-dns-name", "spnego.HeaderFlags.is.dns.name",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_is_flat_name,
      { "is-flat-name", "spnego.HeaderFlags.is.flat.name",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_only_ldap_needed,
      { "only-ldap-needed", "spnego.HeaderFlags.only.ldap.needed",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_avoid_self,
      { "avoid-self", "spnego.HeaderFlags.avoid.self",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_good_timeserv_pref,
      { "good-timeserv-pref", "spnego.HeaderFlags.good.timeserv.pref",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_writable_required,
      { "writable-required", "spnego.HeaderFlags.writable.required",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_timeserv_required,
      { "timeserv-required", "spnego.HeaderFlags.timeserv.required",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_kdc_required,
      { "kdc-required", "spnego.HeaderFlags.kdc.required",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ip_required,
      { "ip-required", "spnego.HeaderFlags.ip.required",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_background_only,
      { "background-only", "spnego.HeaderFlags.background.only",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_pdc_required,
      { "pdc-required", "spnego.HeaderFlags.pdc.required",
        FT_BOOLEAN, 8, NULL, 0x80,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_gc_server_required,
      { "gc-server-required", "spnego.HeaderFlags.gc.server.required",
        FT_BOOLEAN, 8, NULL, 0x40,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_preferred,
      { "ds-preferred", "spnego.HeaderFlags.ds.preferred",
        FT_BOOLEAN, 8, NULL, 0x20,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_ds_required,
      { "ds-required", "spnego.HeaderFlags.ds.required",
        FT_BOOLEAN, 8, NULL, 0x10,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_unused28,
      { "unused28", "spnego.HeaderFlags.unused28",
        FT_BOOLEAN, 8, NULL, 0x08,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_unused29,
      { "unused29", "spnego.HeaderFlags.unused29",
        FT_BOOLEAN, 8, NULL, 0x04,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_unused30,
      { "unused30", "spnego.HeaderFlags.unused30",
        FT_BOOLEAN, 8, NULL, 0x02,
        NULL, HFILL }},
    { &hf_spnego_HeaderFlags_force_rediscovery,
      { "force-rediscovery", "spnego.HeaderFlags.force.rediscovery",
        FT_BOOLEAN, 8, NULL, 0x01,
        NULL, HFILL }},
  };

  /* List of subtrees */
  static int *ett[] = {
    &ett_spnego,
    &ett_spnego_wraptoken,
    &ett_spnego_krb5,
    &ett_spnego_krb5_cfx_flags,

    &ett_spnego_NegotiationToken,
    &ett_spnego_MechTypeList,
    &ett_spnego_NegTokenInit,
    &ett_spnego_NegHints,
    &ett_spnego_NegTokenInit2,
    &ett_spnego_ContextFlags,
    &ett_spnego_NegTokenTarg,
    &ett_spnego_InitialContextToken_U,
    &ett_spnego_HeaderFlags,
    &ett_spnego_IAKERB_HEADER,
  };

  static ei_register_info ei[] = {
    { &ei_spnego_decrypted_keytype, { "spnego.decrypted_keytype", PI_SECURITY, PI_CHAT, "Decrypted keytype", EXPFILL }},
    { &ei_spnego_unknown_header, { "spnego.unknown_header", PI_PROTOCOL, PI_WARN, "Unknown header", EXPFILL }},
  };

  expert_module_t* expert_spnego;

  /* Register protocol */
  proto_spnego = proto_register_protocol(PNAME, PSNAME, PFNAME);

  spnego_handle = register_dissector("spnego", dissect_spnego, proto_spnego);
  spnego_wrap_handle = register_dissector("spnego-wrap", dissect_spnego_wrap, proto_spnego);

  proto_spnego_krb5 = proto_register_protocol("SPNEGO-KRB5", "SPNEGO-KRB5", "spnego-krb5");

  spnego_krb5_handle = register_dissector("spnego-krb5", dissect_spnego_krb5, proto_spnego_krb5);
  spnego_krb5_wrap_handle = register_dissector("spnego-krb5-wrap", dissect_spnego_krb5_wrap, proto_spnego_krb5);

  /* Register fields and subtrees */
  proto_register_field_array(proto_spnego, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_spnego = expert_register_protocol(proto_spnego);
  expert_register_field_array(expert_spnego, ei, array_length(ei));
}


/*--- proto_reg_handoff_spnego ---------------------------------------*/
void proto_reg_handoff_spnego(void) {

  /* Register protocol with GSS-API module */

  gssapi_init_oid("1.3.6.1.5.5.2", proto_spnego, ett_spnego,
                  spnego_handle, spnego_wrap_handle,
                  "SPNEGO - Simple Protected Negotiation");

  /* Register both the one MS created and the real one */
  /*
   * Thanks to Jean-Baptiste Marchand and Richard B Ward, the
   * mystery of the MS KRB5 OID is cleared up. It was due to a library
   * that did not handle OID components greater than 16 bits, and was
   * fixed in Win2K SP2 as well as WinXP.
   * See the archive of <ietf-krb-wg@anl.gov> for the thread topic
   * SPNEGO implementation issues. 3-Dec-2002.
   */
  gssapi_init_oid("1.2.840.48018.1.2.2", proto_spnego_krb5, ett_spnego_krb5,
                  spnego_krb5_handle, spnego_krb5_wrap_handle,
                  "MS KRB5 - Microsoft Kerberos 5");
  gssapi_init_oid("1.2.840.113554.1.2.2", proto_spnego_krb5, ett_spnego_krb5,
                  spnego_krb5_handle, spnego_krb5_wrap_handle,
                  "KRB5 - Kerberos 5");
  gssapi_init_oid("1.2.840.113554.1.2.2.3", proto_spnego_krb5, ett_spnego_krb5,
                  spnego_krb5_handle, spnego_krb5_wrap_handle,
                  "KRB5 - Kerberos 5 - User to User");
  gssapi_init_oid("1.3.6.1.5.2.5", proto_spnego_krb5, ett_spnego_krb5,
                  spnego_krb5_handle, spnego_krb5_wrap_handle,
                  "KRB5 - IAKERB");
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
