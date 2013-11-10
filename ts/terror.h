

#ifndef  terror_h
#define  terror_h


typedef enum rterr_code
{
	rterr_param_notmatch,
	rterr_cst_outofrange,
	rterr_stt_outofrange,
	rterr_obj_unaddable,
	rterr_obj_unsubable,
	rterr_obj_unmulable,
	rterr_obj_undivable,
	rterr_obj_typeerror,
}rterr_code;

#endif
