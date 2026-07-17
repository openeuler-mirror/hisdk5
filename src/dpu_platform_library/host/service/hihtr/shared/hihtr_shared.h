#ifndef __HIHTR_SHARED_H__
#define __HIHTR_SHARED_H__

typedef struct {
    volatile uint64_t hihtr_rq_psn_error_count;
    volatile uint64_t hihtr_rq_nak_error_count;
    volatile uint64_t hihtr_rq_rnr_error_count;
} hihtr_history_cqe_error_count;

typedef struct {
    volatile uint64_t hihtr_event_cq_err;
    volatile uint64_t hihtr_event_qp_fatal;
    volatile uint64_t hihtr_event_qp_req_err;
    volatile uint64_t hihtr_event_qp_access_err;
    volatile uint64_t hihtr_event_port_err;
    volatile uint64_t hihtr_event_srq_err;
    volatile uint64_t hihtr_event_srq_limit_reached;
    volatile uint64_t hihtr_event_qp_last_wqe_reached;
} hihtr_history_event_error_count;

typedef struct {
    hihtr_history_cqe_error_count cqe_error_count;
    hihtr_history_event_error_count event_error_count;
} hihtr_history_error_count;

enum {
	HIHTR_TRP_SUBTYPE_PSN_ERR						= 0x01,
	HIHTR_TRP_SUBTYPE_RNR							= 0x02,
	HIHTR_TRP_SUBTYPE_RNR_EXCEED					= 0x04,
	HIHTR_TRP_SUBTYPE_SGE_LEN_ERR					= 0x08,
	HIHTR_TRP_SUBTYPE_NAK_INVALID_REQUEST			= 0x10,
	HIHTR_TRP_SUBTYPE_NAK_REMOTE_ACCESS_ERROR		= 0x11,
	HIHTR_TRP_SUBTYPE_NAK_REMOTE_OPERATION_ERROR	= 0x12,
	HIHTR_TRP_SUBTYPE_NAK_INVALID_RD_REQUEST		= 0x13,
	HIHTR_TRP_SUBTYPE_NAK_PAYLOAD_POISON			= 0x15,
};

#endif