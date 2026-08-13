// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/character.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/csprng.h>
#include <rudiments/parameterstring.h>
#include <rudiments/process.h>
#include <rudiments/file.h>
#include <rudiments/error.h>

#include <datatypes.h>

#include "firebirdsdl.h"
#include "firebirdsrp.h"

// NOTE:
// Firebird Wire Protocol refers to:
// Carlos Guzman Alvarez, Mark Rotteveel, version 0.18, 18 May 2025
// https://firebirdsql.org/file/documentation/html/en/firebirddocs/wireprotocol/firebird-wire-protocol.html
//
// It only covers protocol version 10 and up, and 11 and up only partially.
// An archived copy of 0.18, an op code coverage matrix, an audit of the
// constants below, and a list of what the document leaves out are at:
// http://trac.firstworks.com/trac/wiki/Firebird%20Wire%20Protocol

// operation codes
// (firebird's P_OP, in numeric order.  the gaps are numbers firebird
// keeps reserved for operations it no longer defines)
#define op_void			0
#define op_connect		1
#define op_exit			2
#define op_accept		3
#define op_reject		4
// 5 op_protocol - obsolete
#define op_disconnect		6
// 7 op_credit, 8 op_continuation - obsolete
#define op_response		9
// 10-18 page server operations - obsolete
#define op_attach		19
#define op_create		20
#define op_detach		21
#define op_compile		22
#define op_start		23
#define op_start_and_send	24
#define op_send			25
#define op_receive		26
#define op_unwind		27
#define op_release		28
#define op_transaction		29
#define op_commit		30
#define op_rollback		31
#define op_prepare		32
#define op_reconnect		33
#define op_create_blob		34
#define op_open_blob		35
#define op_get_segment		36
#define op_put_segment		37
#define op_cancel_blob		38
#define op_close_blob		39
#define op_info_database	40
#define op_info_request		41
#define op_info_transaction	42
#define op_info_blob		43
#define op_batch_segments	44
// 45-47 server manager operations - obsolete
#define op_que_events		48
#define op_cancel_events	49
#define op_commit_retaining	50
#define op_prepare2		51
#define op_event		52
#define op_connect_request	53
#define op_aux_connect		54
#define op_ddl			55
#define op_open_blob2		56
#define op_create_blob2		57
#define op_get_slice		58
#define op_put_slice		59
#define op_slice		60
#define op_seek_blob		61
#define op_allocate_statement	62
#define op_execute		63	// for DDL/DML
#define op_exec_immediate	64
#define op_fetch		65
#define op_fetch_response	66
#define op_free_statement	67
#define op_prepare_statement	68
#define op_set_cursor		69
#define op_info_sql		70
#define op_dummy		71
#define op_response_piggyback	72
#define op_start_and_receive	73
#define op_start_send_and_receive	74
#define op_exec_immediate2	75
#define op_execute2		76	// for Stored procedures
#define op_insert		77
#define op_sql_response		78
#define op_transact		79
#define op_transact_response	80
#define op_drop_database	81
#define op_service_attach	82
#define op_service_detach	83
#define op_service_info		84
#define op_service_start	85
#define op_rollback_retaining	86
#define op_update_account_info	87
#define op_authenticate_user	88
#define op_partial		89
#define op_trusted_auth		90
#define op_cancel		91
#define op_cont_auth		92
#define op_ping			93
#define op_accept_data		94
#define op_abort_aux_connection	95
#define op_crypt		96
#define op_crypt_key_callback	97
#define op_cond_accept		98
#define op_batch_create		99
#define op_batch_msg		100
#define op_batch_exec		101
#define op_batch_rls		102
#define op_batch_cs		103
#define op_batch_regblob	104
#define op_batch_blob_stream	105
#define op_batch_set_bpb	106
#define op_repl_data		107
#define op_repl_req		108
#define op_batch_cancel		109
#define op_batch_sync		110
#define op_info_batch		111
#define op_fetch_scroll		112
#define op_info_cursor		113
#define op_inline_blob		114
#define op_max			115
// arch codes
// (firebird's P_ARCH, plus the pre-firebird InterBase values it has
// dropped but old clients can still send.  9 is arch_sun386 in
// InterBase, but the firebird name is kept here)
#define arch_generic		1
#define arch_apollo		2
#define arch_sun		3
#define arch_vms		4
#define arch_ultrix		5
#define arch_alliant		6
#define arch_msdos		7
#define arch_sun4		8
#define arch_sunx86		9
#define arch_hpux		10
#define arch_hpmpexl		11
#define arch_mac		12
#define arch_macaux		13
#define arch_rt			14
#define arch_mips_ultrix	15
#define arch_hpux_68k		16
#define arch_xenix		17
#define arch_aviion		18
#define arch_sgi		19
#define arch_apollo_dn10k	20
#define arch_cray		21
#define arch_imp		22
#define arch_delta		23
#define arch_sco		24
#define arch_next		25
#define arch_next_386		26
#define arch_m88k		27
#define arch_unixware		28
#define arch_intel_32		29
#define arch_epson		30
#define arch_decosf		31
#define arch_ncr3000		32
#define arch_nt_ppc		33
#define arch_dg_x86		34
#define arch_sco_ev		35
#define arch_linux		36
#define arch_freebsd		37
#define arch_netbsd		38
#define arch_darwin_ppc		39
#define arch_winnt_64		40
#define arch_darwin_x64		41
#define arch_darwin_ppc64	42
#define arch_arm		43
#define arch_winnt_arm64	44
#define arch_max		45

// protocol versions
#define PROTOCOL_VERSION3	3
// 4 supports server management functions
#define PROTOCOL_VERSION4	4
// 5 supports d_float data type
#define PROTOCOL_VERSION5	5
// 6 supports cancel remote events, blob seek, and unknown message type
#define PROTOCOL_VERSION6	6
// 7 supports dsql
#define PROTOCOL_VERSION7	7
// 8 supports includes collapsing first receive into a send, drop db,
// DSQL execute 2, DSQL execute immediate 2, DSQL insert, services, and
// transact request
#define PROTOCOL_VERSION8	8
// 9 includes support for SPX32
#define PROTOCOL_VERSION9	9
// 10 supports warnings, removes requirement for encoding/decoding status codes
#define PROTOCOL_VERSION10	10

// 11 and up set the high bit, to separate firebird from Borland InterBase
#define FB_PROTOCOL_FLAG	0x8000
#define FB_PROTOCOL_MASK	0x7fff

// Firebird holds versions 11 and up in a USHORT as (FB_PROTOCOL_FLAG|n), so
// 11 is 0x800b.  But remote/protocol.cpp marshals p_cnct_version and
// p_acpt_version with xdr_short, which is signed, so 0x800b arrives
// sign-extended as 0xffff800b.  Those fields are read here into a uint32_t,
// so 0xffff8000|n is the right constant to compare against and the
// wrong-looking high bits are correct.  Do not extend this to the type
// fields.  p_cnct_min_type and p_cnct_max_type are marshalled with
// xdr_u_short, not signed, so pflag_compress arrives as plain 0x00000100.

// 11 supports user-auth-related operations
#define PROTOCOL_VERSION11	(0xffff8000|11)
// 12 supports asynchronous calls
#define PROTOCOL_VERSION12	(0xffff8000|12)
// 13 supports auth plugins
#define PROTOCOL_VERSION13	(0xffff8000|13)
// 14 bug-fix
#define PROTOCOL_VERSION14	(0xffff8000|14)
// 15 supports crypt key callback during connect
#define PROTOCOL_VERSION15	(0xffff8000|15)
// 16 supports statement timeouts
#define PROTOCOL_VERSION16	(0xffff8000|16)
// 17 supports op_batch_sync, op_info_batch
#define PROTOCOL_VERSION17	(0xffff8000|17)
// 18 supports op_fetch_scroll, op_info_cursor
#define PROTOCOL_VERSION18	(0xffff8000|18)
// 19 supports op_inline_blob
#define PROTOCOL_VERSION19	(0xffff8000|19)
// 20 supports prepare flags
#define PROTOCOL_VERSION20	(0xffff8000|20)

// the highest version the module negotiates unless the maxprotocolversion
// parameter raises it
// (13 and up answer op_accept_data and drive the auth plugin handshake, which
// no real client has been run against yet, so they stay opt-in)
// this also keeps op_fetch_scroll and op_info_cursor (18) unreachable
#define MAX_PROTOCOL_VERSION	PROTOCOL_VERSION12

// how many offered protocols the connect block can carry
// (10 before firebird 6; anything past this count is ignored)
#define MAX_CNCT_VERSIONS	11

// connect versions
// (InterBase 6 through firebird 2.5 send 2, firebird 3.0 and up send 3.
// 3 says the user id is UTF-8; 2 leaves its encoding undefined)
#define CONNECT_VERSION2	2
#define CONNECT_VERSION3	3

// ptype codes
#define ptype_page		1
#define ptype_rpc		2
#define ptype_batch_send	3
#define ptype_out_of_band	4
#define ptype_lazy_send		5
#define ptype_MASK		0xff

// the highest type the module can negotiate
// (ptype_out_of_band lets the client send out-of-band data, and
// ptype_lazy_send lets it defer op_allocate_statement, op_create_blob and
// op_open_blob, neither of which the module implements)
#define MAX_PROTOCOL_TYPE	ptype_batch_send

// ptype flags
// (the upper byte of the type fields, which must be masked off with
// ptype_MASK before the type itself can be compared)
#define pflag_compress		0x100
#define pflag_win_sspi_nego	0x200

// connect block user id tags
// (1 through 6 are InterBase's.  firebird marks 3 obsolete, but an InterBase
// client can still send it.  7, 8, 10 and 11 arrived with protocol 13)
#define CNCT_user		1
#define CNCT_passwd		2
#define CNCT_ppo		3
#define CNCT_host		4
#define CNCT_group		5
#define CNCT_user_verification	6
#define CNCT_specific_data	7
#define CNCT_plugin_name	8
#define CNCT_login		9
#define CNCT_plugin_list	10
#define CNCT_client_crypt	11

// object handles
#define INVALID_OBJECT		0xffff
#define MAX_OBJCT_HANDLES	65000

// statement flags
#define STMT_NO_BATCH		2
#define STMT_DEFER_EXECUTE	4

// fetch operations
#define fetch_next		0
#define fetch_prior		1
#define fetch_first		2
#define fetch_last		3
#define fetch_absolute		4
#define fetch_relative		5

// cursor flags
#define CURSOR_TYPE_SCROLLABLE	0x01

// database parameters
#define isc_dpb_version1		1
#define isc_dpb_version2		2

#define isc_dpb_cdd_pathname		1
#define isc_dpb_allocation		2
#define isc_dpb_journal			3
#define isc_dpb_page_size		4
#define isc_dpb_num_buffers		5
#define isc_dpb_buffer_length		6
#define isc_dpb_debug			7
#define isc_dpb_garbage_collect		8
#define isc_dpb_verify			9
#define isc_dpb_sweep			10
#define isc_dpb_enable_journal		11
#define isc_dpb_disable_journal		12
#define isc_dpb_dbkey_scope		13
#define isc_dpb_number_of_users		14
#define isc_dpb_trace			15
#define isc_dpb_no_garbage_collect	16
#define isc_dpb_damaged			17
#define isc_dpb_license			18
#define isc_dpb_sys_user_name		19
#define isc_dpb_encrypt_key		20
#define isc_dpb_activate_shadow		21
#define isc_dpb_sweep_interval		22
#define isc_dpb_delete_shadow		23
#define isc_dpb_force_write		24
#define isc_dpb_begin_log		25
#define isc_dpb_quit_log		26
#define isc_dpb_no_reserve		27
#define isc_dpb_user_name		28
#define isc_dpb_password		29
#define isc_dpb_password_enc		30
#define isc_dpb_sys_user_name_enc	31
#define isc_dpb_interp			32
#define isc_dpb_online_dump		33
#define isc_dpb_old_file_size		34
#define isc_dpb_old_num_files		35
#define isc_dpb_old_file		36
#define isc_dpb_old_start_page		37
#define isc_dpb_old_start_seqno		38
#define isc_dpb_old_start_file		39
#define isc_dpb_drop_walfile		40
#define isc_dpb_old_dump_id		41
#define isc_dpb_wal_backup_dir		42
#define isc_dpb_wal_chkptlen		43
#define isc_dpb_wal_numbufs		44
#define isc_dpb_wal_bufsize		45
#define isc_dpb_wal_grp_cmt_wait	46
#define isc_dpb_lc_messages		47
#define isc_dpb_lc_ctype		48
#define isc_dpb_cache_manager		49
#define isc_dpb_shutdown		50
#define isc_dpb_online			51
#define isc_dpb_shutdown_delay		52
#define isc_dpb_reserved		53
#define isc_dpb_overwrite		54
#define isc_dpb_sec_attach		55
#define isc_dpb_disable_wal		56
#define isc_dpb_connect_timeout		57
#define isc_dpb_dummy_packet_interval	58
#define isc_dpb_gbak_attach		59
#define isc_dpb_sql_role_name		60
#define isc_dpb_set_page_buffers	61
#define isc_dpb_working_directory	62
#define isc_dpb_sql_dialect		63
#define isc_dpb_set_db_readonly		64
#define isc_dpb_set_db_sql_dialect	65
#define isc_dpb_gfix_attach		66
#define isc_dpb_gstat_attach		67
#define isc_dpb_set_db_charset		68
#define isc_dpb_gsec_attach		69
#define isc_dpb_address_path		70
#define isc_dpb_process_id		71
#define isc_dpb_no_db_triggers		72
#define isc_dpb_trusted_auth		73
#define isc_dpb_process_name		74
#define isc_dpb_trusted_role		75
#define isc_dpb_org_filename		76
#define isc_dpb_utf8_filename		77
#define isc_dpb_ext_call_depth		78
#define isc_dpb_auth_block		79
#define isc_dpb_client_version		80
#define isc_dpb_remote_protocol		81
#define isc_dpb_host_name		82
#define isc_dpb_os_user			83
#define isc_dpb_specific_auth_data	84
#define isc_dpb_auth_plugin_list	85
#define isc_dpb_auth_plugin_name	86
#define isc_dpb_config			87
#define isc_dpb_nolinger		88
#define isc_dpb_reset_icu		89
#define isc_dpb_map_attach		90
#define isc_dpb_session_time_zone	91
#define isc_dpb_set_db_replica		92
#define isc_dpb_set_bind		93
#define isc_dpb_decfloat_round		94
#define isc_dpb_decfloat_traps		95
#define isc_dpb_clear_map		96
#define isc_dpb_upgrade_db		97
#define isc_dpb_parallel_workers	100
#define isc_dpb_worker_attach		101
#define isc_dpb_owner			102
#define isc_dpb_max_blob_cache_size	103
#define isc_dpb_max_inline_blob_size	104
#define isc_dpb_search_path		105
#define isc_dpb_blr_request_search_path	106
#define isc_dpb_gbak_restore_has_schema	107

// tags inside an isc_dpb_address_path value
#define isc_dpb_address			1
#define isc_dpb_addr_protocol		1
#define isc_dpb_addr_endpoint		2
#define isc_dpb_addr_flags		3
#define isc_dpb_addr_crypt		4

// bits in an isc_dpb_addr_flags value
#define isc_dpb_addr_flag_conn_compressed	0x01
#define isc_dpb_addr_flag_conn_encrypted	0x02

// bits in an isc_dpb_verify value
#define isc_dpb_pages			1
#define isc_dpb_records			2
#define isc_dpb_indices			4
#define isc_dpb_transactions		8
#define isc_dpb_no_update		16
#define isc_dpb_repair			32
#define isc_dpb_ignore			64

// bits in an isc_dpb_shutdown value
#define isc_dpb_shut_cache		0x1
#define isc_dpb_shut_attachment		0x2
#define isc_dpb_shut_transaction	0x4
#define isc_dpb_shut_force		0x8
#define isc_dpb_shut_mode_mask		0x70
#define isc_dpb_shut_default		0x0
#define isc_dpb_shut_normal		0x10
#define isc_dpb_shut_multi		0x20
#define isc_dpb_shut_single		0x30
#define isc_dpb_shut_full		0x40

// values of an isc_dpb_set_db_replica value
#define isc_dpb_replica_none		0
#define isc_dpb_replica_read_only	1
#define isc_dpb_replica_read_write	2

// common structural codes
#define isc_info_end			1
#define isc_info_truncated		2
#define isc_info_error			3
#define isc_info_data_not_ready		4
#define isc_info_length			126
#define isc_info_flag_end		127

// db information items
#define isc_info_db_id			4
#define isc_info_reads			5
#define isc_info_writes			6
#define isc_info_fetches		7
#define isc_info_marks			8
#define isc_info_implementation		11
#define isc_info_isc_version		12
#define isc_info_base_level		13
#define isc_info_page_size		14
#define isc_info_num_buffers		15
#define isc_info_limbo			16
#define isc_info_current_memory		17
#define isc_info_max_memory		18
#define isc_info_window_turns		19
#define isc_info_license		20
#define isc_info_allocation		21
#define isc_info_attachment_id		22
#define isc_info_read_seq_count		23
#define isc_info_read_idx_count		24
#define isc_info_insert_count		25
#define isc_info_update_count		26
#define isc_info_delete_count		27
#define isc_info_backout_count		28
#define isc_info_purge_count		29
#define isc_info_expunge_count		30
#define isc_info_sweep_interval		31
#define isc_info_ods_version		32
#define isc_info_ods_minor_version	33
#define isc_info_no_reserve		34
#define isc_info_logfile		35
#define isc_info_cur_logfile_name	36
#define isc_info_cur_log_part_offset	37
#define isc_info_num_wal_buffers	38
#define isc_info_wal_buffer_size	39
#define isc_info_wal_ckpt_length	40
#define isc_info_wal_cur_ckpt_interval	41
#define isc_info_wal_prv_ckpt_fname	42
#define isc_info_wal_prv_ckpt_poffset	43
#define isc_info_wal_recv_ckpt_fname	44
#define isc_info_wal_recv_ckpt_poffset	45
#define isc_info_wal_grpc_wait_usecs	47
#define isc_info_wal_num_io		48
#define isc_info_wal_avg_io_size	49
#define isc_info_wal_num_commits	50
#define isc_info_wal_avg_grpc_size	51
#define isc_info_forced_writes		52
#define isc_info_user_names		53
#define isc_info_page_errors		54
#define isc_info_record_errors		55
#define isc_info_bpage_errors		56
#define isc_info_dpage_errors		57
#define isc_info_ipage_errors		58
#define isc_info_ppage_errors		59
#define isc_info_tpage_errors		60
#define isc_info_set_page_buffers	61
#define isc_info_db_sql_dialect		62
#define isc_info_db_read_only		63
#define isc_info_db_size_in_pages	64
#define frb_info_att_charset		101
#define isc_info_db_class		102
#define isc_info_firebird_version	103
#define isc_info_oldest_transaction	104
#define isc_info_oldest_active		105
#define isc_info_oldest_snapshot	106
#define isc_info_next_transaction	107
#define isc_info_db_provider		108
#define isc_info_active_transactions	109
#define isc_info_active_tran_count	110
#define isc_info_creation_date		111
#define isc_info_db_file_size		112
#define fb_info_page_contents		113
#define fb_info_implementation		114
#define fb_info_page_warns		115
#define fb_info_record_warns		116
#define fb_info_bpage_warns		117
#define fb_info_dpage_warns		118
#define fb_info_ipage_warns		119
#define fb_info_ppage_warns		120
#define fb_info_tpage_warns		121
#define fb_info_pip_errors		122
#define fb_info_pip_warns		123
#define fb_info_pages_used		124
#define fb_info_pages_free		125
// 126 and 127 are the structural codes above, so no db info item uses them
#define fb_info_ses_idle_timeout_db	129
#define fb_info_ses_idle_timeout_att	130
#define fb_info_ses_idle_timeout_run	131
#define fb_info_conn_flags		132
#define fb_info_crypt_key		133
#define fb_info_crypt_state		134
#define fb_info_statement_timeout_db	135
#define fb_info_statement_timeout_att	136
// 137 is how a client asks which protocol the server settled on.  it reports
// the bare version, not the 0x8000-flagged one the handshake carries.
#define fb_info_protocol_version	137
#define fb_info_crypt_plugin		138
#define fb_info_creation_timestamp_tz	139
#define fb_info_wire_crypt		140
#define fb_info_features		141
#define fb_info_next_attachment		142
#define fb_info_next_statement		143
#define fb_info_db_guid			144
#define fb_info_db_file_id		145
#define fb_info_replica_mode		146
#define fb_info_username		147
#define fb_info_sqlrole			148
#define fb_info_parallel_workers	149
#define fb_info_wire_out_packets	150
#define fb_info_wire_in_packets		151
#define fb_info_wire_out_bytes		152
#define fb_info_wire_in_bytes		153
#define fb_info_wire_snd_packets	154
#define fb_info_wire_rcv_packets	155
#define fb_info_wire_snd_bytes		156
#define fb_info_wire_rcv_bytes		157
#define fb_info_wire_roundtrips		158
#define fb_info_max_blob_cache_size	159
#define fb_info_max_inline_blob_size	160
#define fb_info_counts_scope_att	161
#define fb_info_counts_scope_db		162

// transaction information items
#define isc_info_tra_id			4
#define isc_info_tra_oldest_interesting	5
#define isc_info_tra_oldest_snapshot	6
#define isc_info_tra_oldest_active	7
#define isc_info_tra_isolation		8
#define isc_info_tra_access		9
#define isc_info_tra_lock_timeout	10
#define fb_info_tra_dbpath		11
#define fb_info_tra_snapshot_number	12

// isc_info_tra_isolation responses
#define isc_info_tra_consistency	1
#define isc_info_tra_concurrency	2
#define isc_info_tra_read_committed	3

// isc_info_tra_read_committed options
#define isc_info_tra_no_rec_version	0
#define isc_info_tra_rec_version	1
#define isc_info_tra_read_consistency	2

// isc_info_tra_access responses
#define isc_info_tra_readonly		0
#define isc_info_tra_readwrite		1

// blob information items
#define isc_info_blob_num_segments	4
#define isc_info_blob_max_segment	5
#define isc_info_blob_total_length	6
#define isc_info_blob_type		7

// transaction parameters
#define isc_tpb_version1                  1
#define isc_tpb_version3                  3
#define isc_tpb_consistency               1
#define isc_tpb_concurrency               2
#define isc_tpb_shared                    3
#define isc_tpb_protected                 4
#define isc_tpb_exclusive                 5
#define isc_tpb_wait                      6
#define isc_tpb_nowait                    7
#define isc_tpb_read                      8
#define isc_tpb_write                     9
#define isc_tpb_lock_read                 10
#define isc_tpb_lock_write                11
#define isc_tpb_verb_time                 12
#define isc_tpb_commit_time               13
#define isc_tpb_ignore_limbo              14
#define isc_tpb_read_committed	          15
#define isc_tpb_autocommit                16
#define isc_tpb_rec_version               17
#define isc_tpb_no_rec_version            18
#define isc_tpb_restart_requests          19
#define isc_tpb_no_auto_undo              20
#define isc_tpb_lock_timeout              21
#define isc_tpb_read_consistency          22
#define isc_tpb_at_snapshot_number        23
#define isc_tpb_auto_release_temp_blobid  24
#define isc_tpb_lock_table_schema         25

// blob parameters
#define isc_bpb_version1		1
#define isc_bpb_source_type		1
#define isc_bpb_target_type		2
#define isc_bpb_type			3
#define isc_bpb_source_interp		4
#define isc_bpb_target_interp		5
#define isc_bpb_filter_parameter	6
#define isc_bpb_storage			7

// values of an isc_bpb_type value
#define isc_bpb_type_segmented		0x0
#define isc_bpb_type_stream		0x1

// values of an isc_bpb_storage value
#define isc_bpb_storage_main		0x0
#define isc_bpb_storage_temp		0x2

// batch parameters
// (IBatch in firebird's IdlFbInterfaces.h - a batch parameter buffer is a
// version byte that has to be BATCH_version1, then items of a tag byte, a
// 4-byte little-endian length, and that many value bytes)
#define BATCH_version1			1

#define BATCH_tag_multierror		1
#define BATCH_tag_record_counts		2
#define BATCH_tag_buffer_bytes_size	3
#define BATCH_tag_blob_policy		4
#define BATCH_tag_detailed_errors	5

// what firebird's DsqlBatch caps those two at
#define BATCH_hard_buffer_limit		(256U*1024U*1024U)
#define BATCH_detailed_limit		(64U*4U)

// how many errors firebird's DsqlBatch details when the batch parameter
// buffer didn't say
#define BATCH_detailed_default		64U

// what a blob's bytes are aligned to in an op_batch_blob_stream, and what a
// segment header is aligned to inside one
// (BLOB_STREAM_ALIGN and BATCH_SEGHDR_ALIGN in firebird's IdlFbInterfaces.h)
#define BATCH_blob_stream_align		4
#define BATCH_blob_seghdr_align		2

// how much of the client's stream buffer a blob header and a segment header
// take up
// (Rsr::BatchStream::SIZEOF_BLOB_HEAD in firebird's src/remote/remote.h -
// an 8-byte blob id and two 4-byte lengths - and the sizeof(USHORT) that
// xdr_blob_stream() steps a segment header by.  a segment header is wider
// than that on the wire - see parseBatchBlobStream())
#define BATCH_blob_hdr_size		16
#define BATCH_blob_seghdr_size		2

// op_info_batch item codes
// (the INF_* enum in firebird's IdlFbInterfaces.h - a distinct namespace
// from the BATCH_tag_* parameter-buffer values above, used only in an
// op_info_batch response)
#define BATCH_inf_buffer_bytes_size	10
#define BATCH_inf_data_size		11
#define BATCH_inf_blobs_size		12
#define BATCH_inf_element_alignment	13
#define BATCH_inf_blob_header		14

// event parameters
#define EPB_version1			1

// free statement flags
// (flag bits, not an enum)
#define DSQL_close	1
#define DSQL_drop	2
#define DSQL_unprepare	4

// cancel kinds
// (the p_co_kind of op_cancel.  a client must never send fb_cancel_abort;
// it just closes the socket instead)
#define fb_cancel_disable	1
#define fb_cancel_enable	2
#define fb_cancel_raise		3
#define fb_cancel_abort		4

// sql information items
#define isc_info_sql_select			4
#define isc_info_sql_bind			5
#define isc_info_sql_num_variables		6
#define isc_info_sql_describe_vars		7
#define isc_info_sql_describe_end		8
#define isc_info_sql_sqlda_seq			9
#define isc_info_sql_message_seq		10
#define isc_info_sql_type			11
#define isc_info_sql_sub_type			12
#define isc_info_sql_scale			13
#define isc_info_sql_length			14
#define isc_info_sql_null_ind			15
#define isc_info_sql_field			16
#define isc_info_sql_relation			17
#define isc_info_sql_owner			18
#define isc_info_sql_alias			19
#define isc_info_sql_sqlda_start		20
#define isc_info_sql_stmt_type			21
#define isc_info_sql_get_plan			22
#define isc_info_sql_records			23
#define isc_info_sql_batch_fetch		24
#define isc_info_sql_relation_alias		25
#define isc_info_sql_explain_plan		26
#define isc_info_sql_stmt_flags			27
#define isc_info_sql_stmt_timeout_user		28
#define isc_info_sql_stmt_timeout_run		29
#define isc_info_sql_stmt_blob_align		30
#define isc_info_sql_exec_path_blr_bytes	31
#define isc_info_sql_exec_path_blr_text		32
#define isc_info_sql_relation_schema		33

// sql statement types
// (a separate value space that happens to share the prefix.  these are the
// values isc_info_sql_stmt_type returns, not items that can be asked for)
#define isc_info_sql_stmt_select		1
#define isc_info_sql_stmt_insert		2
#define isc_info_sql_stmt_update		3
#define isc_info_sql_stmt_delete		4
#define isc_info_sql_stmt_ddl			5
#define isc_info_sql_stmt_get_segment		6
#define isc_info_sql_stmt_put_segment		7
#define isc_info_sql_stmt_exec_procedure	8
#define isc_info_sql_stmt_start_trans		9
#define isc_info_sql_stmt_commit		10
#define isc_info_sql_stmt_rollback		11
#define isc_info_sql_stmt_select_for_upd	12
#define isc_info_sql_stmt_set_generator		13
#define isc_info_sql_stmt_savepoint		14

// isc_info_sql_stmt_flags bits
#define FB_STMT_HAS_CURSOR	0x1
#define FB_STMT_REPEAT_EXECUTE	0x2

// isc_info_sql_records sub-items
// (a separate value space again - these appear inside the
// isc_info_sql_records value, not at the top level of a reply)
#define isc_info_req_select_count	13
#define isc_info_req_insert_count	14
#define isc_info_req_update_count	15
#define isc_info_req_delete_count	16

// sql data types
// (ibase.h's SQL_*, which the module can't include.  the low bit means the
// value carries a null indicator, so every type here is even.)
#define SQL_VARYING	448
#define SQL_TEXT	452
#define SQL_DOUBLE	480
#define SQL_FLOAT	482
#define SQL_LONG	496
#define SQL_SHORT	500
#define SQL_TIMESTAMP	510
#define SQL_BLOB	520
#define SQL_D_FLOAT	530
#define SQL_ARRAY	540
#define SQL_QUAD	550
#define SQL_TYPE_TIME	560
#define SQL_TYPE_DATE	570
#define SQL_INT64	580
#define SQL_BOOLEAN	32764

// blr
// (the module reads two kinds of blr - the message blr that describes a row
// or a parameter set, and the request blr an op_compile carries.  each has a
// value space of its own, read in a context of its own, so a number below
// can mean one thing as a message item and another as a request verb.)

// message layout verbs and the data type codes their items carry
#define blr_begin	2
#define blr_message	4
#define blr_version4	4
#define blr_version5	5
#define blr_short	7
#define blr_long	8
#define blr_quad	9
#define blr_float	10
#define blr_d_float	11
#define blr_sql_date	12
#define blr_sql_time	13
#define blr_text	14
#define blr_text2	15
#define blr_int64	16
#define blr_blob2	17
#define blr_bool	23
#define blr_double	27
#define blr_timestamp	35
#define blr_varying	37
#define blr_varying2	38
#define blr_cstring	40
#define blr_cstring2	41
#define blr_eoc		76
#define blr_end		255

// request verbs
// (the subset parseBlrRequest() understands - see the comment there)
#define blr_assignment	1
#define blr_for		7
#define blr_receive	12
#define blr_send	14
#define blr_literal	21
#define blr_field	23
#define blr_fid		24
#define blr_parameter	25
#define blr_parameter2	41
#define blr_null	45
#define blr_eql		47
#define blr_neq		48
#define blr_gtr		49
#define blr_geq		50
#define blr_lss		51
#define blr_leq		52
#define blr_containing	53
#define blr_starting	55
#define blr_between	56
#define blr_or		57
#define blr_and		58
#define blr_not		59
#define blr_missing	61
#define blr_like	63
#define blr_rse		67
#define blr_first	68
#define blr_sort	70
#define blr_boolean	71
#define blr_ascending	72
#define blr_descending	73
#define blr_relation	74
#define blr_rid		75
#define blr_join_type	80
#define blr_relation2	146

// the widest request blr parseBlrRequest() will take
// (the requests isql's SHOW commands compile stay well inside these - one
// that doesn't is refused rather than translated into a query that only
// half means what it said)
#define FIREBIRD_MAX_BLR_CONTEXTS	16
#define FIREBIRD_MAX_BLR_MESSAGES	8
#define FIREBIRD_MAX_BLR_RELATIONS	2
#define FIREBIRD_MAX_BLR_PARAMS		32
#define FIREBIRD_MAX_BLR_DEPTH		32

// how many 1/10000ths of a second an ISC_TIME counts to the second
#define FIREBIRD_TIME_PRECISION	10000

// the largest day count decodeDate() will take
// (firebird's own ceiling is 31 December 9999, and its decode multiplies the
// count by four, so anything past that has to be refused rather than
// overflowed)
#define MAX_FIREBIRD_DATE	5373484

// how much of a blob to read from the backend at a time
// (65535 is the largest segment firebird can store, since isc_put_segment's
// length is a USHORT.  the firebird backend's read loop stops short when a
// stored segment ends, so asking for exactly this many gets back exactly one
// stored segment per call - see getLobFieldSegment() in the firebird
// connection module.)
#define BLOB_SEGMENT_SIZE	65535

// how many bytes one character can take
#define MAX_BYTES_PER_CHAR	4

// how many bytes of fetched blob a session will hold at once
// (a fetched blob has to be buffered whole - see bufferBlob() - and a client
// is free to fetch a great many rows before it opens any of the ids it got
// back, so without a ceiling a select of a million rows with a blob column is
// an out-of-memory.  the oldest blobs nothing is reading are dropped to stay
// under this, and their ids stop resolving.)
#define MAX_BLOB_BUFFER		(64*1024*1024)

// how many bytes of fetched array elements a session will hold at once
// (an array is buffered whole at fetch time for the same reason a blob is -
// see bufferArray() - and gets the same ceiling and the same trimming)
#define MAX_ARRAY_BUFFER	(64*1024*1024)

// how many elements getArrayFieldSlice() is asked for at a time
#define ARRAY_SLICE_ELEMENTS	4096

// the high word of an array id
// (a blob id and an array id both reach the module as a quad, and nothing
// about a bind parameter's quad says which of the two it is - see
// readMessageValue().  a blob id's high word is 0, so arrays are given a
// high word of their own and the two can't be mistaken for each other.)
#define ARRAY_ID_HIGH		1

// the states of an op_get_segment response
// (a bare 0, 1 or 2 in the response's object handle.  isc_segment and
// isc_segstr_eof never go on the wire - firebird's client raises those itself,
// from these.)
#define BLOB_MORE		0
#define BLOB_PARTIAL_SEGMENT	1
#define BLOB_EOF		2

// the seek modes of op_seek_blob
// (bare numbers - firebird defines no isc_seek_* constants)
#define BLOB_SEEK_START		0
#define BLOB_SEEK_RELATIVE	1
#define BLOB_SEEK_END		2

// how wide a bind variable says it is
// (a bind whose type the module couldn't work out describes as a string, and
// this is the width it claims - big enough for anything a client is likely to
// send, small enough that a client sizing a buffer per parameter doesn't
// notice)
#define FIREBIRD_BIND_LENGTH	4000

// status vector items
#define isc_arg_end		0
#define isc_arg_gds		1
#define isc_arg_string		2
#define isc_arg_cstring		3
#define isc_arg_number		4
#define isc_arg_interpreted	5
#define isc_arg_vms		6
#define isc_arg_unix		7
#define isc_arg_domain		8
#define isc_arg_dos		9
#define isc_arg_mpexl		10
#define isc_arg_mpexl_ipc	11
#define isc_arg_next_mach	15
#define isc_arg_netware		16
#define isc_arg_win32		17
#define isc_arg_warning		18
#define isc_arg_sql_state	19

// gds error codes
#define isc_arith_except		335544321
#define isc_bad_req_handle		335544327
#define isc_bad_segstr_handle		335544328
#define isc_bad_segstr_id		335544329
#define isc_bad_trans_handle		335544332
#define isc_convert_error		335544334
#define isc_infunk			335544341
#define isc_io_error			335544344
#define isc_no_dup			335544349
#define isc_no_meta_update		335544351
#define isc_read_only_trans		335544361
#define isc_wish_list			335544378
#define isc_imp_exc			335544381
#define isc_random			335544382
#define isc_sqlerr			335544436
#define isc_invalid_sdl			335544456
#define isc_login			335544472
#define isc_bad_stmt_handle		335544485
#define isc_dsql_error			335544569
#define isc_dsql_command_err		335544570
#define isc_dsql_cursor_err		335544572
#define isc_dsql_datatype_err		335544573
#define isc_dsql_cursor_close_err	335544577
#define isc_dsql_field_err		335544578
#define isc_dsql_relation_err		335544580
#define isc_dsql_sqlda_err		335544583
#define isc_dsql_token_unk_err		335544634
#define isc_unique_key_violation	335544665
#define isc_io_open_err			335544734
#define isc_service_att_err		335544792
#define isc_bad_batch_handle		335545159
#define isc_batch_param_version		335545182
#define isc_batch_open			335545184
#define isc_batch_too_big		335545198

// dyn error codes
// (a second code space, carried after isc_no_meta_update in the compound
// status vector a real server sends when ddl fails.  one code per ddl verb,
// each with a message template of its own - "CREATE TABLE @1 failed" and so
// on - whose @1 is the object name, sent as the isc_arg_string element that
// follows the code.  the numbers come from iberror.h and the templates from
// firebird.msg, both shipped with firebird itself - see ddlDynCode().)
#define isc_dsql_create_proc_failed		336397265
#define isc_dsql_alter_proc_failed		336397266
#define isc_dsql_create_alter_proc_failed	336397267
#define isc_dsql_drop_proc_failed		336397268
#define isc_dsql_recreate_proc_failed		336397269
#define isc_dsql_create_trigger_failed		336397270
#define isc_dsql_alter_trigger_failed		336397271
#define isc_dsql_create_alter_trigger_failed	336397272
#define isc_dsql_drop_trigger_failed		336397273
#define isc_dsql_recreate_trigger_failed	336397274
#define isc_dsql_create_domain_failed		336397277
#define isc_dsql_alter_domain_failed		336397278
#define isc_dsql_drop_domain_failed		336397279
#define isc_dsql_create_except_failed		336397280
#define isc_dsql_alter_except_failed		336397281
#define isc_dsql_recreate_except_failed		336397283
#define isc_dsql_drop_except_failed		336397284
#define isc_dsql_create_sequence_failed		336397285
#define isc_dsql_create_table_failed		336397286
#define isc_dsql_alter_table_failed		336397287
#define isc_dsql_drop_table_failed		336397288
#define isc_dsql_recreate_table_failed		336397289
#define isc_dsql_create_view_failed		336397298
#define isc_dsql_alter_view_failed		336397299
#define isc_dsql_create_alter_view_failed	336397300
#define isc_dsql_recreate_view_failed		336397301
#define isc_dsql_drop_view_failed		336397302
#define isc_dsql_drop_sequence_failed		336397303
#define isc_dsql_recreate_sequence_failed	336397304
#define isc_dsql_drop_index_failed		336397305
#define isc_dsql_drop_role_failed		336397308
#define isc_dsql_drop_user_failed		336397309
#define isc_dsql_create_role_failed		336397310
#define isc_dsql_alter_role_failed		336397311
#define isc_dsql_alter_index_failed		336397312
#define isc_dsql_create_index_failed		336397316
#define isc_dsql_create_user_failed		336397317
#define isc_dsql_alter_user_failed		336397318
#define isc_dsql_alter_sequence_failed		336397323
#define isc_dsql_create_generator_failed	336397324

// how much room the module keeps for a ddl statement's object name
// (a firebird identifier is 31 bytes in dialect 3 through 3.0 and 63 from 4.0
// on, so this is generous either way, and a longer one is truncated rather
// than overrunning)
#define FIREBIRD_MAX_OBJECT_NAME_LENGTH	255

// what the module answers isc_database_info with
// (connect() caps protocol negotiation at 12, so the module presents itself
// as a firebird 2.5-era server - see MAX_PROTOCOL_VERSION)
#define FIREBIRD_PAGE_SIZE		4096
#define FIREBIRD_NUM_BUFFERS		75
#define FIREBIRD_ODS_VERSION		11
#define FIREBIRD_ODS_MINOR_VERSION	2
#define FIREBIRD_SQL_DIALECT		3
// NONE - no transliteration
#define FIREBIRD_CHARACTER_SET		0
#define FIREBIRD_SWEEP_INTERVAL		20000
// classic access - one server process per attachment, like sqlr-connection
#define FIREBIRD_DB_CLASS		13
// isc_info_db_code_firebird
#define FIREBIRD_DB_PROVIDER		4
// pages allocated, which is also the database size in pages, so a client that
// multiplies either one by the page size above gets the same file size
#define FIREBIRD_DB_SIZE_IN_PAGES	512
#define FIREBIRD_CURRENT_MEMORY		1048576
#define FIREBIRD_MAX_MEMORY		2097152
// SQL Relay can't ask the backend when its database was created, and a client
// that shows the date needs a real one rather than day zero of the modified
// julian day epoch
#define FIREBIRD_CREATION_YEAR		2000
#define FIREBIRD_CREATION_MONTH		1
#define FIREBIRD_CREATION_DAY		1

// authentication methods
// (which dpb item the password came out of, or which auth plugin protocol 13
// and up negotiated - see attach() and selectAuthPlugin())
#define FIREBIRD_CLEARTEXT	"firebird_cleartext"
#define FIREBIRD_LEGACY		"firebird_legacy"
#define FIREBIRD_SRP		"firebird_srp"
#define FIREBIRD_SRP256		"firebird_srp256"

// the auth plugins protocol 13 and up can negotiate, most preferred first
// (the same three a stock firebird offers, in the same order - its
// AuthServer setting defaults to "Srp256, Srp, Legacy_Auth")
#define FIREBIRD_PLUGIN_SRP256	"Srp256"
#define FIREBIRD_PLUGIN_SRP	"Srp"
#define FIREBIRD_PLUGIN_LEGACY	"Legacy_Auth"
#define FIREBIRD_PLUGIN_LIST	"Srp256,Srp,Legacy_Auth"

// how many random bytes the srp server private key is built from
// (RemotePassword::makePrivate() - srp.cpp:75-83 - uses SRP_KEY_SIZE, which
// srp.h:108 sets to 128)
#define FIREBIRD_SRP_PRIVATE_KEY_SIZE	128

// SEGMENT_DATA_SIZE - server.cpp:511.  A CNCT_specific_data item is capped at
// 255 bytes, of which the first is the sequence number, so each item after
// the first starts 254 bytes further into the data.
#define FIREBIRD_CNCT_SEGMENT_SIZE	254

// connection type
#define P_REQ_async	1

// the largest counted string or buffer the module will read
// (firebird's api types these lengths as a short - see isc_attach_database
// and isc_database_info in ibase.h - and its own decoder folds a
// sign-extended 0xffffffff back down to 0xffff)
#define MAX_CSTRING_LENGTH	65535

// one field of a message, as the blr describes it
struct sqlrfirebirdfield {
	byte_t		blrtype;
	int16_t		scale;
	uint16_t	subtype;
	uint16_t	length;
};

// what one bind variable describes as, when describeBinds() could work it out
struct sqlrfirebirdbind {
	uint16_t	coltype;
	uint32_t	colsize;
	uint32_t	colscale;
};

// what one output column describes as, when describeOutputColumns() had to
// work the result set's shape out on a cursor of its own
struct sqlrfirebirdprobecolumn {
	char		*name;
	char		*table;
	uint16_t	coltype;
	uint32_t	colsize;
	uint32_t	colscale;
};

// one field of a message, as it came off the wire, before anything has been
// done with it
struct sqlrfirebirdvalue {
	bool		isnull;
	int64_t		intval;
	double		dblval;
	uint32_t	dateval;
	uint32_t	timeval;
	char		*strval;
	uint32_t	strvallen;
	bool		isdate;
	bool		istime;
	bool		isblob;
	// whether the id came in as a blr_quad rather than a blr_blob2, and
	// so might name an array instead of a blob - see readMessageValue()
	bool		isquad;
	uint32_t	blobhigh;
	uint32_t	bloblow;
};

// one message the client added to a batch with op_batch_msg
struct sqlrfirebirdbatchmessage {
	sqlrfirebirdvalue	*values;
	uint16_t		valuecount;
};

// a blob the client made known to a batch, either by registering one it
// already built (op_batch_regblob) or by streaming one into the batch
// (op_batch_blob_stream).  the id the messages refer to is the client's own,
// which is why it can't just be the module's blob id.
struct sqlrfirebirdbatchblob {
	uint32_t	temphigh;
	uint32_t	templow;
	// what the module's own blob is called - see newBlob()
	uint32_t	blobid;
};

// one message of a batch that failed, held until the completion state that
// reports it has been written
struct sqlrfirebirdbatcherror {
	uint32_t	position;
	uint32_t	gdscode;
	int32_t		sqlcode;
	// NULL when the gds code says it all
	char		*message;
};

// what the module knows about a batch the client created on a statement
struct sqlrfirebirdbatch {
	bool			open;
	// the message format op_batch_create described
	sqlrfirebirdfield	*fields;
	uint16_t		fieldcount;
	// what the batch parameter buffer asked for
	// (the blob policy isn't kept - a real firebird rewrites it to
	// BLOB_STREAM whatever the client sent, so a batch is always streamed)
	bool			multierror;
	bool			recordcounts;
	uint32_t		detailederrors;
	uint32_t		buffersize;
	// what the bpb op_batch_set_bpb left behind, for blobs the batch
	// registers later
	bool			blobsegmented;
	// the messages op_batch_msg has queued, none of them run yet
	linkedlist< sqlrfirebirdbatchmessage * >	messages;
	uint64_t		queuedbytes;
	// the blobs the messages can refer to, by the client's id
	linkedlist< sqlrfirebirdbatchblob * >	blobs;
	// where the last op_batch_blob_stream left off, since a blob can
	// span one op and carry on in the next - see parseBatchBlobStream()
	// (the module's own id of the blob being filled, 0 between blobs)
	uint32_t		blobstreamblobid;
	uint32_t		blobstreamremaining;
	uint32_t		blobstreambpbremaining;
	uint32_t		blobstreamsegremaining;
	bool			blobstreamsegmented;
	// a parameter buffer or a segment that arrived in pieces, held until
	// the last piece makes it whole
	bytebuffer		blobstreambpb;
	bytebuffer		blobstreamdata;
};

// how one item of a compiled request's message gets filled
struct sqlrfirebirdblrslot {
	// which column of the translated select fills the item, or -1 when
	// the request assigned it a constant instead
	int32_t		column;
	// the constant, as text, or NULL when the item takes a column or
	// nothing at all
	char		*literal;
	// which column of the translated select the item is the null
	// indicator for, or -1 when the item isn't an indicator at all
	int32_t		indicator;
};

// what op_compile made of a request blr
// (the boolean, sort and first clauses are kept as the sql they translated
// to rather than as a tree of their own - nothing needs them in any other
// form, and the walk that reads them can emit the text as it goes)
struct sqlrfirebirdblrrequest {
	bool		compiled;
	// the relations the rse pulls from, indexed by the context number a
	// field reference names them by
	char		*relations[FIREBIRD_MAX_BLR_CONTEXTS];
	uint8_t		relationcount;
	// the messages the request declared, by message number
	sqlrfirebirdfield	*msgfields[FIREBIRD_MAX_BLR_MESSAGES];
	uint16_t		msgfieldcount[FIREBIRD_MAX_BLR_MESSAGES];
	// which message carries the rows back, and which one the client
	// sends its input parameters in
	uint16_t	outmsg;
	uint16_t	inmsg;
	bool		hasinmsg;
	// how the send inside the for loop fills the output message, and how
	// the send after the loop does - the second is what tells the client
	// the stream ended
	sqlrfirebirdblrslot	*rowslots;
	sqlrfirebirdblrslot	*eofslots;
	uint16_t		slotcount;
	bool			haseof;
	// which item of the input message each ? in the query binds to, in
	// the order the ?s appear
	uint16_t	inparams[FIREBIRD_MAX_BLR_PARAMS];
	uint16_t	inparamcount;
	// the select the request translated to
	char		*query;
	uint32_t	querylen;
};

// where parseBlrRequest()'s walk has got to
struct sqlrfirebirdblrwalk {
	stringbuffer	selectclause;
	stringbuffer	fromclause;
	stringbuffer	whereclause;
	stringbuffer	orderbyclause;
	stringbuffer	firstclause;
	uint16_t	selectcount;
	// whether the walk is inside the for loop, which is what tells the
	// send that carries a row from the send that ends the stream
	bool		inloop;
	// the message items the send being walked fills
	sqlrfirebirdblrslot	*slots;
	uint16_t	depth;
};

// what the module knows about a statement the client allocated
struct sqlrfirebirdstatement {
	uint32_t		stmttype;
	bool			prepared;
	bool			preexecuted;
	bool			cursoropen;
	char			*cursorname;
	// the blr comes only with the first fetch of a cursor
	sqlrfirebirdfield	*outfields;
	uint16_t		outfieldcount;
	// NULL when describeBinds() couldn't work the types out
	sqlrfirebirdbind	*binds;
	uint16_t		bindcount;
	bool			bindsdescribed;
	// NULL unless describeOutputColumns() had to probe for the shape of
	// the result set
	sqlrfirebirdprobecolumn	*probecols;
	uint32_t		probecolcount;
	// only open between an op_batch_create and the op_batch_rls that
	// ends it
	sqlrfirebirdbatch	batch;
	// only filled in when op_compile made the statement, rather than
	// op_allocate_statement
	sqlrfirebirdblrrequest	request;
};

// a blob the session is holding, either built by the client or fetched
struct sqlrfirebirdblob {
	uint32_t	id;
	// non-zero only between an open and the close that ends it
	uint32_t	handle;
	bool		iswrite;
	bytebuffer	data;
	bytebuffer	seglengths;
	uint32_t	segcount;
	uint32_t	maxseglength;
	// how far a read has got
	uint64_t	readpos;
	uint32_t	readseg;
	uint64_t	readsegstart;
	// what the bpb asked for, answered back as isc_info_blob_type
	bool		isstream;
};

// ISC_ARRAY_DESC, laid out the way ibase.h lays it out.  When the backend
// is firebird, that's what getArrayFieldDescriptor() hands back, and there's
// no way to reach ibase.h from here - a protocol module builds without any
// database client library - so the layout is mirrored instead.  It's only
// read when the descriptor is exactly this big, and the ISC_ARRAY_DESC
// layout has been fixed since interbase, so a descriptor that came from
// something else won't be mistaken for one of these.
struct sqlrfirebirdarraybound {
	int16_t		lower;
	int16_t		upper;
};

struct sqlrfirebirdarraydesc {
	byte_t		dtype;
	char		scale;
	uint16_t	length;
	char		fieldname[32];
	char		relationname[32];
	int16_t		dimensions;
	int16_t		flags;
	sqlrfirebirdarraybound	bounds[SQLRFIREBIRDSDL_MAX_DIMENSIONS];
};

// an array a fetch pulled out of the backend, held until the client asks
// for a slice of it
struct sqlrfirebirdarray {
	uint32_t	id;
	bytebuffer	data;
	// what one element looks like, which is what says how to read the
	// bytes back - see appendArrayElement()
	byte_t		elementtype;
	int8_t		elementscale;
	uint32_t	elementsize;
	uint64_t	elementcount;
	// the array's own bounds, out of its descriptor.  0 dimensions means
	// the backend had no descriptor to give, and then the slice the
	// client asks for has to be taken to cover the whole array.
	uint16_t	dimensions;
	int32_t		lower[SQLRFIREBIRDSDL_MAX_DIMENSIONS];
	int32_t		upper[SQLRFIREBIRDSDL_MAX_DIMENSIONS];
};

class SQLRSERVER_DLLSPEC sqlrprotocol_firebird : public sqlrprotocol {
	public:
		sqlrprotocol_firebird(sqlrservercontroller *cont,
							domnode *parameters);
		virtual	~sqlrprotocol_firebird();

		clientsessionexitstatus_t	clientSession(
							filedescriptor *cs);

	private:
		void	init();
		void	free();

		bool	initialHandshake();
		bool	connect();
		bool	attach();

		void	parseUserId(const byte_t *userid, uint32_t useridlen);
		bool	clientSupportsPlugin(const char *plugin);
		bool	selectAuthPlugin(bytebuffer *acceptdata);
		bool	srpChallenge(bytebuffer *acceptdata);
		bool	acceptData(uint32_t acptversion,
					uint32_t acptarchtype,
					uint32_t acpttype,
					uint32_t *byteswritten);
		bool	continueAuthentication();
		void	setAuthMethodFromPlugin(const char *plugin);

		void	successStatusVector();
		void	errorStatusVector(uint32_t gdscode);
		void	openErrorStatusVector(const char *file);
		bool	genericResponse(const char *title,
						uint32_t objecthandle,
						uint64_t blobid,
						const byte_t *buffer,
						uint32_t bufferlen,
						uint32_t *sv,
						const char **svstr,
						uint8_t svlen);

		bool	authenticate();
		bool	validateDatabase();

		bool	getOpCode();
		bool	detach();
		bool	create();
		bool	dropDatabase();
		bool	infoDatabase();
		bool	disconnect();
		bool	transaction();
		bool	commit();
		bool	rollback();
		bool	commitRetaining();
		bool	rollbackRetaining();
		bool	execImmediate();
		bool	execImmediate2();
		bool	prepare();
		bool	prepare2();
		bool	transactionInfo();
		bool	allocateStatement();
		bool	freeStatement();
		bool	prepareStatement();
		bool	execute();
		bool	execute2();
		bool	fetch();
		bool	compile();
		bool	startAndReceive();
		bool	startSendAndReceive();
		bool	setCursor();
		bool	infoSql();
		bool	createBlob();
		bool	createBlob2();
		bool	createBlobCommon(const char *title, bool hasbpb);
		bool	openBlob();
		bool	openBlob2();
		bool	openBlobCommon(const char *title, bool hasbpb);
		bool	getSegment();
		bool	putSegment();
		bool	batchSegment();
		bool	putSegmentCommon(const char *title, bool batch);
		bool	seekBlob();
		bool	cancelBlob();
		bool	closeBlob();
		bool	infoBlob();
		bool	infoBatch();
		bool	getSlice();
		bool	putSlice();
		bool	cancel();
		bool	batchCreate();
		bool	batchMsg();
		bool	batchExec();
		bool	batchRls();
		bool	batchCancel();
		bool	batchSync();
		bool	batchSetBpb();
		bool	batchRegBlob();
		bool	batchBlobStream();
		bool	serviceAttach();
		bool	serviceDetach();
		bool	serviceStart();
		bool	serviceInfo();
		bool	connectRequest();
		bool	queEvents();
		bool	cancelEvents();
		bool	sendNotImplementedError();
		bool	sendServiceAttachError();

		bool	errorResponse(const char *title, uint32_t gdscode);
		bool	errorResponse(const char *title,
					uint32_t gdscode,
					const char *sqlstate,
					int32_t sqlcode,
					const char *message,
					uint32_t messagesize);
		bool	sendCursorError(const char *title,
					sqlrservercursor *cursor,
					bool preparing);
		const char	*sqlStateForSqlCode(int32_t sqlcode);

		sqlrfirebirdstatement	*getStatement(uint32_t stmthandle,
						sqlrservercursor **cursor);
		void	clearStatement(uint16_t cursorid);
		void	clearStatements();

		void	initBatch(sqlrfirebirdbatch *batch);
		void	clearBatch(sqlrfirebirdbatch *batch);
		void	clearBatchMessages(sqlrfirebirdbatch *batch);
		void	clearBatchBlobs(sqlrfirebirdbatch *batch);
		bool	parseBatchPb(const byte_t *pb,
					uint32_t pblen,
					sqlrfirebirdbatch *batch,
					uint32_t *gdscode);
		sqlrfirebirdbatchblob	*getBatchBlob(
					sqlrfirebirdbatch *batch,
					uint32_t high,
					uint32_t low);
		void	setBatchBlob(sqlrfirebirdbatch *batch,
					uint32_t high,
					uint32_t low,
					uint32_t blobid);
		bool	parseBatchBlobStream(sqlrfirebirdbatch *batch,
					uint32_t streamlen,
					uint32_t *bytesread);
		bool	execBatchMessage(sqlrservercursor *cursor,
					sqlrfirebirdbatch *batch,
					sqlrfirebirdbatchmessage *msg,
					uint32_t position,
					uint32_t *affected,
					linkedlist< sqlrfirebirdbatcherror *>
								*errors);
		bool	batchCompletionState(const char *title,
					uint32_t stmthandle,
					sqlrfirebirdbatch *batch,
					uint32_t reccount,
					uint32_t *updates,
					linkedlist< sqlrfirebirdbatcherror *>
								*errors);
		bool	batchRelease(const char *title, bool cancel);

		sqlrfirebirdblob	*newBlob();
		uint32_t	newBlobHandle();
		sqlrfirebirdblob	*getBlobById(uint32_t high, uint32_t low);
		sqlrfirebirdblob	*getBlobByHandle(uint32_t blobhandle);
		void	removeBlob(sqlrfirebirdblob *blob);
		void	clearBlobs();
		void	trimBlobs();
		void	parseBpb(const byte_t *bpb,
					uint32_t bpblen,
					sqlrfirebirdblob *blob);
		void	appendBlobSegment(sqlrfirebirdblob *blob,
					const byte_t *value,
					uint32_t valuelen);
		void	rewindBlob(sqlrfirebirdblob *blob, uint64_t position);
		void	bufferBlob(sqlrservercursor *cursor,
					uint32_t col,
					const char *value,
					uint64_t valuesize,
					bool lob,
					uint32_t *id);

		sqlrfirebirdarray	*newArray();
		sqlrfirebirdarray	*getArrayById(uint32_t high,
							uint32_t low);
		void	clearArrays();
		void	trimArrays();
		void	bufferArray(sqlrservercursor *cursor,
					uint32_t col,
					uint32_t *id);
		static uint32_t	arrayElementSize(byte_t blrtype,
						uint16_t length);
		bool	readSliceElement(byte_t blrtype,
					uint32_t elementsize,
					byte_t *element,
					uint32_t *bytesread);
		bool	drainSliceElements(byte_t blrtype,
					uint32_t elementsize,
					uint64_t elementcount,
					uint32_t *bytesread);
		bool	appendArrayElement(stringbuffer *output,
					byte_t blrtype,
					int8_t scale,
					const byte_t *element,
					uint32_t elementsize);
		bool	writeSliceElement(byte_t blrtype,
					uint32_t elementsize,
					const byte_t *element,
					uint32_t *byteswritten);

		uint32_t	statementType(const char *query);
		bool	isTransactionStatement(uint32_t stmttype);
		bool	isWriteStatement(uint32_t stmttype);
		const char	*matchDdlKeyword(const char *query,
						const char *keyword);
		void	parseDdlObjectName(const char *query);
		bool	ddlDynCode(const char *query, uint32_t *dyncode);
		bool	readOnlyMetaUpdateResponse(const char *title,
						uint32_t dyncode,
						const char *objname);
		bool	runTransactionStatement(uint32_t stmttype);

		bool	prepareOrExecImmediate(bool execimmediate);
		bool	runPreparedQuery(bool execimmediate,
					uint32_t stmthandle,
					const char *query,
					uint32_t querylen,
					const byte_t *items,
					uint32_t itemslen);
		bool	executeStatement(bool isexecute2);
		bool	runOnCursor(sqlrservercursor *cursor,
					const char *title,
					const char *query,
					uint32_t querylen,
					sqlrfirebirdfield *outfields,
					uint16_t outfieldcount);
		bool	sendSqlResponse(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount);
		bool	sendFetchResponse(sqlrservercursor *cursor,
					sqlrfirebirdstatement *stmt,
					uint32_t msgcount);

		bool	startRequest(bool send);
		bool	sendStartResponse(uint32_t msgnumber);
		bool	sendRequestRows(uint32_t reqhandle,
					uint32_t incarnation,
					sqlrservercursor *cursor,
					sqlrfirebirdblrrequest *req);
		bool	sendRequestMessage(uint32_t reqhandle,
					uint32_t incarnation,
					uint32_t msgcount,
					sqlrservercursor *cursor,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrslot *slots);
		bool	writeRequestMessage(sqlrservercursor *cursor,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrslot *slots,
					uint32_t *byteswritten);
		bool	readRequestMessage(sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					sqlrfirebirdvalue *values,
					uint32_t *bytesread);

		void	initBlrRequest(sqlrfirebirdblrrequest *req);
		void	clearBlrRequest(sqlrfirebirdblrrequest *req);
		static sqlrfirebirdblrslot	*newBlrSlots(uint16_t count);
		static void	clearBlrSlots(sqlrfirebirdblrslot *slots,
						uint16_t count);
		bool	parseBlrRequest(const byte_t *blr,
					uint32_t blrlen,
					sqlrfirebirdblrrequest *req,
					uint32_t *gdscode);
		bool	parseBlrStatement(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode);
		bool	parseBlrAssignment(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode);
		bool	parseBlrRse(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode);
		bool	parseBlrRelation(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode);
		bool	parseBlrSort(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode);
		bool	parseBlrBoolean(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					stringbuffer *sql,
					uint16_t depth,
					uint32_t *gdscode);
		bool	parseBlrValue(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					stringbuffer *sql,
					stringbuffer *raw,
					byte_t *kind,
					uint32_t *gdscode);
		bool	parseBlrLiteral(const byte_t **blr,
					const byte_t *end,
					stringbuffer *sql,
					stringbuffer *raw,
					uint32_t *gdscode);
		bool	buildBlrRequestQuery(sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode);

		bool	readBlr(sqlrfirebirdfield **fields,
					uint16_t *fieldcount,
					const char *name,
					uint32_t *bytesread,
					uint32_t *gdscode);
		bool	parseBlr(const byte_t *blr,
					uint32_t blrlen,
					sqlrfirebirdfield **fields,
					uint16_t *fieldcount,
					uint32_t *gdscode);
		bool	parseBlrItems(const byte_t **blr,
					const byte_t *end,
					uint16_t itemcount,
					bool paired,
					sqlrfirebirdfield **fields,
					uint16_t *fieldcount,
					uint32_t *gdscode);
		bool	readMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *bytesread);
		bool	readMessageNullBits(uint16_t fieldcount,
					byte_t **nullbits,
					uint32_t *bytesread);
		bool	readMessageFields(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					const byte_t *nullbits,
					uint32_t *bytesread);
		bool	readMessageValue(const sqlrfirebirdfield *fld,
					const byte_t *nullbits,
					uint16_t index,
					bool nullindicator,
					sqlrfirebirdvalue *value,
					uint32_t *bytesread);
		void	bindMessageValue(memorypool *bindpool,
					sqlrserverbindvar *bv,
					uint16_t bindindex,
					const sqlrfirebirdfield *fld,
					const sqlrfirebirdvalue *val);
		bool	writeMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *byteswritten);
		bool	writeField(sqlrservercursor *cursor,
					uint32_t col,
					const sqlrfirebirdfield *fld,
					const char *value,
					uint64_t valuesize,
					bool lob,
					bool null,
					uint32_t *byteswritten);
		bool	writeOpaque(const byte_t *val,
					uint32_t len,
					const char *name,
					uint32_t *byteswritten);
		bool	writeInt64(uint64_t val,
					const char *name,
					uint32_t *byteswritten);
		bool	readInt64(uint64_t *val,
					const char *name,
					uint32_t *bytesread);
		bool	readOpaque(byte_t *val,
					uint32_t len,
					const char *name,
					uint32_t *bytesread);

		uint16_t	sqlType(uint16_t coltype);
		uint16_t	sqlLength(uint16_t sqltype, uint32_t colsize);
		int16_t		sqlSubType(uint16_t coltype);

		bool	appendInfoBare(byte_t item);
		bool	appendInfoDescribe(sqlrservercursor *cursor,
					sqlrfirebirdstatement *stmt,
					bool bind,
					uint32_t start,
					const byte_t *items,
					uint32_t itemslen);
		bool	appendInfoRecords(sqlrservercursor *cursor,
					uint32_t stmttype);
		bool	buildSqlInfo(sqlrservercursor *cursor,
					sqlrfirebirdstatement *stmt,
					const byte_t *items,
					uint32_t itemslen);

		int64_t		scaledInteger(const char *value, int16_t scale);
		uint16_t	splitNumbers(const char *value,
						int32_t *parts,
						uint16_t maxparts);
		uint32_t	encodeDate(int32_t year,
						int32_t month,
						int32_t day);
		uint32_t	encodeTime(int32_t hour,
						int32_t minute,
						int32_t second,
						int32_t fraction);
		void	decodeDate(uint32_t date,
					int16_t *year,
					int16_t *month,
					int16_t *day);
		void	decodeTime(uint32_t time,
					int16_t *hour,
					int16_t *minute,
					int16_t *second,
					int32_t *microsecond);

		// debugging aid, wired in by hand when reverse-engineering
		// a wire exchange; no callers
		void	keepReading(int32_t sec, int32_t usec);

		void	readStringFromBuffer(const byte_t *in,
						uint32_t len,
						const char *name,
						char **buf);

		bool	readInt(uint32_t *val,
					const char *name,
					uint32_t *bytesread);
		bool	readInt(uint32_t *val,
					const char *name,
					uint32_t expected,
					uint32_t *bytesread);
		bool	readString(char **val,
					const char *name,
					uint32_t *bytesread);
		bool	readString(char **val,
					uint32_t *len,
					const char *name,
					uint32_t *bytesread);
		bool	readBuffer(byte_t **val,
					const char *name,
					uint32_t *bytesread);
		bool	readBuffer(byte_t **val,
					uint32_t *len,
					const char *name,
					uint32_t *bytesread);
		bool	readBytes(bytebuffer *val,
					uint32_t len,
					const char *name,
					uint32_t *bytesread);
		bool	readPadding(uint32_t *bytesread);

		uint32_t	foldSignExtendedLength(uint32_t len,
							const char *name);

		void	describeBinds(sqlrservercursor *cursor,
					sqlrfirebirdstatement *stmt,
					const byte_t *items,
					uint32_t itemslen);
		bool	buildBindProbe(sqlrservercursor *cursor,
					stringbuffer *probe,
					uint16_t *bindcount);
		bool	isBindMarker(const char *value);

		bool	describeOutputColumns(sqlrservercursor *cursor,
					sqlrfirebirdstatement *stmt);
		void	clearProbeColumns(sqlrfirebirdstatement *stmt);

		void	fixupRespBufferLen();

		bool	writeInt(uint32_t val,
					const char *name,
					uint32_t *byteswritten);
		bool	writeBuffer(const byte_t *val,
					uint32_t len,
					const char *name,
					uint32_t *byteswritten);
		bool	writeStatusVector(uint32_t *sv,
					const char **svstr,
					uint8_t svlen,
					uint32_t *byteswritten);

		bool	appendInfoItem(byte_t item,
					const byte_t *value,
					uint16_t valuelen);
		bool	appendInfoInt(byte_t item, uint32_t value);
		bool	appendInfoByte(byte_t item, byte_t value);
		bool	appendInfoStrings(byte_t item,
					const char * const *values,
					byte_t valuecount);
		bool	appendInfoCountedString(byte_t item,
					const char *value);
		bool	appendInfoTimestamp(byte_t item,
					int32_t year,
					int32_t month,
					int32_t day,
					int32_t hour,
					int32_t minute,
					int32_t second);
		bool	appendInfoError(byte_t item);

		void	debugSystemError();
		void	debugOpCode(const char *name, uint32_t opcode);
		void	debugArchType(uint32_t archtype);
		void	debugConnectVersion(uint32_t connectversion);
		void	debugProtocolVersion(uint32_t protoversion);
		void	debugProtocolType(const char *title,
						uint32_t protocoltype);
		void	debugUserId(const byte_t *userid,
						uint32_t useridlen);
		void	debugDpbVersion(byte_t dpbversion);
		void	debugDpbParam(byte_t dpbparam);
		void	debugDbInfoItem(byte_t dbinfoitem);
		void	debugTpbVersion(byte_t tpbversion);
		void	debugTpbParam(byte_t tpbparam);
		void	debugStatusVector(uint32_t *sv,
						const char **svstr,
						uint8_t svlen);

		uint32_t	maxquerysize;
		uint16_t	maxbindcount;

		// the highest protocol version negotiation will accept
		uint32_t	maxprotocolversion;

		filedescriptor	*clientsock;

		uint32_t	opcode;

		uint32_t	protocolversion;

		char		*db;
		char		*username;
		char		*password;
		// which dpb item the password came out of, or which auth
		// plugin was negotiated (not owned)
		const char	*authmethod;

		// protocol 13+ auth handshake state...
		// CNCT_login from the connect block
		char		*clientlogin;
		// the plugin the module told the client to continue with
		char		*authplugin;
		// the plugin the client led with, and the plugins it said
		// it has
		char		*clientplugin;
		char		*clientpluginlist;
		// the client's first plugin-specific message - for srp, its
		// public key A as hex text
		char		*authclientdata;
		// the srp inputs that have to survive the round trip between
		// the accept and the attach, both hex text
		char		*srpserverprivatekey;
		char		*srpsalt;

		char		*wd;
		uint32_t	dbhandle;

		uint32_t	statusvector[20];
		// the vector's string arguments, indexed alongside it, a
		// non-null entry marking a string element (not owned)
		const char	*statusvectorstr[20];
		uint8_t		statusvectorlen;

		uint32_t	trhandle;
		bool		intransaction;
		bool		trautocommit;
		bool		trreadonly;
		// the isolation level actually honored for the current
		// transaction, as a tpb byte (isc_tpb_consistency,
		// isc_tpb_concurrency or isc_tpb_read_committed), or 0 if
		// none was requested/honored and the connection's default
		// applies
		byte_t		trisolevel;

		uint16_t	maxcursorcount;
		sqlrfirebirdstatement	*statements;

		char		**bindvarnames;
		int16_t		*bindvarnamesizes;

		// the blobs the session is holding, oldest first
		linkedlist< sqlrfirebirdblob * >	blobs;
		uint64_t	blobbytes;
		uint32_t	nextblobid;
		uint32_t	nextblobhandle;

		// the arrays the session is holding, oldest first
		linkedlist< sqlrfirebirdarray * >	arrays;
		uint64_t	arraybytes;
		uint32_t	nextarrayid;

		// staging buffer for reading a blob out of the backend,
		// allocated on the first blob a session fetches
		char		*lobbuffer;

		// a bind named a blob the session doesn't have
		bool		badblobid;

		// backs the text statusvectorstr doesn't own
		stringbuffer	errormessage;
		char		errorsqlstate[6];
		// the object name ddlDynCode() parsed out of a ddl statement,
		// which readOnlyMetaUpdateResponse() points the vector at
		char		ddlobjectname[FIREBIRD_MAX_OBJECT_NAME_LENGTH+1];

		bytebuffer	respbuffer;

		uint32_t	respbufferlen;
};


sqlrprotocol_firebird::sqlrprotocol_firebird(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrprotocol(cont,parameters) {

	clientsock=NULL;

	// maxprotocolversion - the highest wire protocol version negotiation
	// will accept.  13 and up run the auth plugin handshake (Srp256, Srp
	// or Legacy_Auth) and the packed sql message format, and are opt-in
	// until they've been proven against real clients.  Anything else, and
	// anything unparsable, leaves the ceiling where it has always been.
	uint32_t	configmaxpv=(uint32_t)charstring::convertToInteger(
			parameters->getAttributeValue("maxprotocolversion"));
	if (!configmaxpv || configmaxpv>20) {
		maxprotocolversion=MAX_PROTOCOL_VERSION;
	} else if (configmaxpv<=PROTOCOL_VERSION10) {
		// 10 and below are plain numbers, 11 and up carry the 0x8000
		// bit - see the PROTOCOL_VERSION defines above
		maxprotocolversion=configmaxpv;
	} else {
		maxprotocolversion=0xffff8000|configmaxpv;
	}

	debugStart("parameters");
	if (getDebug()) {
		stdoutput.printf("	maxprotocolversion: %u\n",
						maxprotocolversion);
	}
	debugEnd();

	maxquerysize=cont->getConfig()->getMaxQuerySize();
	maxbindcount=cont->getConfig()->getMaxBindCount();
	maxcursorcount=cont->getConfig()->getMaxCursors();

	// per-statement state, indexed by cursor id
	statements=new sqlrfirebirdstatement[maxcursorcount];
	for (uint16_t i=0; i<maxcursorcount; i++) {
		statements[i].cursorname=NULL;
		statements[i].outfields=NULL;
		statements[i].binds=NULL;
		statements[i].bindcount=0;
		statements[i].bindsdescribed=false;
		statements[i].probecols=NULL;
		statements[i].probecolcount=0;
		initBatch(&statements[i].batch);
		initBlrRequest(&statements[i].request);
	}

	lobbuffer=NULL;

	// the wire format binds by position, so the names are the positions
	bindvarnames=new char *[maxbindcount];
	bindvarnamesizes=new int16_t[maxbindcount];
	for (uint16_t i=0; i<maxbindcount; i++) {
		charstring::printf(&bindvarnames[i],"?%d",i+1);
		bindvarnamesizes[i]=charstring::getLength(bindvarnames[i]);
	}

	init();
}

sqlrprotocol_firebird::~sqlrprotocol_firebird() {
	free();
	delete[] lobbuffer;
	delete[] statements;
	for (uint16_t i=0; i<maxbindcount; i++) {
		delete[] bindvarnames[i];
	}
	delete[] bindvarnames;
	delete[] bindvarnamesizes;
}

void sqlrprotocol_firebird::init() {
	protocolversion=0;
	db=NULL;
	username=NULL;
	password=NULL;
	authmethod=NULL;
	clientlogin=NULL;
	authplugin=NULL;
	clientplugin=NULL;
	clientpluginlist=NULL;
	authclientdata=NULL;
	srpserverprivatekey=NULL;
	srpsalt=NULL;
	wd=NULL;
	dbhandle=0;
	trhandle=0;
	intransaction=false;
	trautocommit=false;
	trreadonly=false;
	trisolevel=0;
	blobbytes=0;
	nextblobid=0;
	nextblobhandle=0;
	badblobid=false;
	arraybytes=0;
	nextarrayid=0;
	errorsqlstate[0]='\0';
	ddlobjectname[0]='\0';
	respbufferlen=0;
	for (uint16_t i=0; i<maxcursorcount; i++) {
		statements[i].stmttype=0;
		statements[i].prepared=false;
		statements[i].preexecuted=false;
		statements[i].cursoropen=false;
		statements[i].cursorname=NULL;
		statements[i].outfields=NULL;
		statements[i].outfieldcount=0;
		statements[i].binds=NULL;
		statements[i].bindcount=0;
		statements[i].bindsdescribed=false;
		statements[i].probecols=NULL;
		statements[i].probecolcount=0;
		initBatch(&statements[i].batch);
		initBlrRequest(&statements[i].request);
	}
}

void sqlrprotocol_firebird::free() {
	delete[] db;
	delete[] username;
	delete[] password;
	delete[] clientlogin;
	delete[] authplugin;
	delete[] clientplugin;
	delete[] clientpluginlist;
	delete[] authclientdata;
	delete[] srpserverprivatekey;
	delete[] srpsalt;
	delete[] wd;
	clearStatements();
	clearBlobs();
	clearArrays();
}

clientsessionexitstatus_t sqlrprotocol_firebird::clientSession(
						filedescriptor *cs) {

	clientsock=cs;

	// Set up the socket...
	clientsock->setTranslateByteOrder(true);
	clientsock->setNaglesAlgorithmEnabled(false);
	clientsock->setSocketReadBufferSize(65536);
	clientsock->setSocketWriteBufferSize(65536);
	clientsock->setReadBufferSize(65536);
	clientsock->setWriteBufferSize(65536);

	// Reinit session-local data...
	free();
	init();

	// state/status variables...
	bool				endsession=true;
	clientsessionexitstatus_t	status=CLIENTSESSIONEXITSTATUS_ERROR;

	// perform the initial handshake...
	if (initialHandshake()) {

		// run session-start queries, now that the client is
		// authenticated
		cont->beginSession();

		// loop, getting and executing requests
		bool	loop=true;
		do {

			// get the request...
			if (!getOpCode()) {
				status=
				CLIENTSESSIONEXITSTATUS_CLOSED_CONNECTION;
				break;
			}

			// execute the request
			switch (opcode) {
				case op_detach:
					loop=detach();
					break;
				case op_create:
					loop=create();
					break;
				case op_drop_database:
					loop=dropDatabase();
					break;
				case op_info_database:
					loop=infoDatabase();
					break;
				case op_disconnect:
					loop=disconnect();
					break;
				case op_transaction:
					loop=transaction();
					break;
				case op_commit:
					loop=commit();
					break;
				case op_rollback:
					loop=rollback();
					break;
				case op_commit_retaining:
					loop=commitRetaining();
					break;
				case op_rollback_retaining:
					loop=rollbackRetaining();
					break;
				case op_exec_immediate:
					loop=execImmediate();
					break;
				case op_exec_immediate2:
					loop=execImmediate2();
					break;
				case op_prepare:
					loop=prepare();
					break;
				case op_prepare2:
					loop=prepare2();
					break;
				case op_info_transaction:
					loop=transactionInfo();
					break;
				case op_allocate_statement:
					loop=allocateStatement();
					break;
				case op_free_statement:
					loop=freeStatement();
					break;
				case op_prepare_statement:
					loop=prepareStatement();
					break;
				case op_execute:
					loop=execute();
					break;
				case op_execute2:
					loop=execute2();
					break;
				case op_fetch:
					loop=fetch();
					break;
				case op_compile:
					loop=compile();
					break;
				case op_start_and_receive:
					loop=startAndReceive();
					break;
				case op_start_send_and_receive:
					loop=startSendAndReceive();
					break;
				case op_set_cursor:
					loop=setCursor();
					break;
				case op_info_sql:
					loop=infoSql();
					break;
				case op_create_blob:
					loop=createBlob();
					break;
				case op_create_blob2:
					loop=createBlob2();
					break;
				case op_open_blob:
					loop=openBlob();
					break;
				case op_open_blob2:
					loop=openBlob2();
					break;
				case op_get_segment:
					loop=getSegment();
					break;
				case op_put_segment:
					loop=putSegment();
					break;
				case op_batch_segments:
					loop=batchSegment();
					break;
				case op_info_blob:
					loop=infoBlob();
					break;
				case op_seek_blob:
					loop=seekBlob();
					break;
				case op_cancel_blob:
					loop=cancelBlob();
					break;
				case op_close_blob:
					loop=closeBlob();
					break;
				case op_get_slice:
					loop=getSlice();
					break;
				case op_put_slice:
					loop=putSlice();
					break;
				case op_cancel:
					loop=cancel();
					break;
				case op_batch_create:
					loop=batchCreate();
					break;
				case op_batch_msg:
					loop=batchMsg();
					break;
				case op_batch_exec:
					loop=batchExec();
					break;
				case op_batch_rls:
					loop=batchRls();
					break;
				case op_batch_cancel:
					loop=batchCancel();
					break;
				case op_batch_sync:
					loop=batchSync();
					break;
				case op_batch_set_bpb:
					loop=batchSetBpb();
					break;
				case op_batch_regblob:
					loop=batchRegBlob();
					break;
				case op_batch_blob_stream:
					loop=batchBlobStream();
					break;
				case op_info_batch:
					loop=infoBatch();
					break;
				case op_service_attach:
					loop=serviceAttach();
					break;
				case op_service_detach:
					loop=serviceDetach();
					break;
				case op_service_start:
					loop=serviceStart();
					break;
				case op_service_info:
					loop=serviceInfo();
					break;
				// event notification's second socket and the
				// event itself to deliver on it don't exist -
				// see the comment above connectRequest()
				case op_connect_request:
					loop=connectRequest();
					break;
				case op_que_events:
					loop=queEvents();
					break;
				case op_cancel_events:
					loop=cancelEvents();
					break;

				// known, but not implemented yet
				case op_exit:
				case op_start:
				case op_start_and_send:
				case op_send:
				case op_receive:
				case op_unwind:
				case op_release:
				// reconnecting to a limbo transaction needs
				// a prepare to have succeeded first, and none
				// ever do
				case op_reconnect:
				case op_info_request:
				case op_aux_connect:
				case op_ddl:
				case op_dummy:
				case op_insert:
				case op_transact:
				case op_update_account_info:
				case op_authenticate_user:
				case op_partial:
				case op_trusted_auth:
				case op_cont_auth:
				case op_ping:
				case op_abort_aux_connection:
				case op_crypt:
				case op_crypt_key_callback:
				case op_repl_data:
				case op_repl_req:
				// the scroll ops below need backward and
				// absolute fetches, and the server api has
				// only forward ones (fetchRow, nextRow,
				// skipRow, skipRows), so they stay stubbed
				case op_fetch_scroll:
				case op_info_cursor:
					loop=sendNotImplementedError();
					break;

				default:
					if (getDebug()) {
						stdoutput.printf(
							"unrecognized "
							"op code: 0x%02x\n",
							opcode);
					}
					loop=sendNotImplementedError();
					break;
			}

		} while (loop);
	}

	// close the client connection
	cont->closeClientConnection(0);

	// end the session if necessary
	if (endsession) {
		cont->endSession();
	}

	// return the status
	return status;
}

bool sqlrprotocol_firebird::initialHandshake() {
	return connect() && attach();
}

bool sqlrprotocol_firebird::connect() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		op_connect
	// 	int32_t		op_attach
	// 	int32_t		connect version
	// 	int32_t		arch type
	// 	int32_t		db length
	// 	char[]		db path or alias
	// 	int32_t		count of protocol versions understood
	// 	int32_t		user id length
	// 	byte_t[]	user id
	//
	// 	// protocols...
	// 	int32_t		protocol version
	// 	int32_t		arch type
	// 	int32_t		minimum type
	// 	int32_t		maximum type
	// 	int32_t		preference weight
	// 	...
	// }

	debugStart("connect");

	uint32_t	bytesread=0;

	// get op_connect
	if (!readInt(&opcode,"connect op code",op_connect,&bytesread)) {
		return false;
	}
	debugOpCode("connect op code",opcode);

	// get op_attach
	if (!readInt(&opcode,"attach op code",op_attach,&bytesread)) {
		return false;
	}
	debugOpCode("attach op code",opcode);

	// get connect version
	uint32_t	connectversion=0;
	if (!readInt(&connectversion,"connect version",&bytesread)) {
		return false;
	}
	debugConnectVersion(connectversion);

	// get arch type
	uint32_t	archtype=0;
	if (!readInt(&archtype,"arch type",&bytesread)) {
		return false;
	}
	debugArchType(archtype);

	// get db
	if (!readString(&db,"db",&bytesread)) {
		return false;
	}

	// get protocol count
	uint32_t	protocount=0;
	if (!readInt(&protocount,"protocol count",&bytesread)) {
		return false;
	}

	// get user identification
	uint32_t	useridlen=0;
	byte_t		*userid=NULL;
	if (!readBuffer(&userid,&useridlen,"user id",&bytesread)) {
		delete[] userid;
		return false;
	}
	debugUserId(userid,useridlen);
	parseUserId(userid,useridlen);
	delete[] userid;

	// get protocols, keeping the best one we can speak
	bool		accepted=false;
	uint32_t	acptversion=0;
	uint32_t	acptarchtype=arch_generic;
	uint32_t	acpttype=0;
	uint32_t	acptweight=0;
	for (uint32_t i=0; i<protocount; i++) {

		if (getDebug()) {
			stdoutput.printf("	protocol %u...\n",i);
		}

		// get protocol version
		uint32_t	protoversion=0;
		if (!readInt(&protoversion,"protocol version",&bytesread)) {
			return false;
		}
		debugProtocolVersion(protoversion);

		// get arch type
		uint32_t	protoarchtype=0;
		if (!readInt(&protoarchtype,"arch type",&bytesread)) {
			return false;
		}
		debugArchType(protoarchtype);

		// get minimum type
		uint32_t	mintype=0;
		if (!readInt(&mintype,"min type",&bytesread)) {
			return false;
		}
		debugProtocolType("min type",mintype);

		// get maximum type
		uint32_t	maxtype=0;
		if (!readInt(&maxtype,"max type",&bytesread)) {
			return false;
		}
		debugProtocolType("max type",maxtype);

		// get preference weight
		uint32_t	prefwt=0;
		if (!readInt(&prefwt,"preference weight",&bytesread)) {
			return false;
		}

		// only the first MAX_CNCT_VERSIONS count, but every one the
		// client sent still has to be read, or the rest would be
		// misread as the next packet
		// (firebird does the same - see FB25
		// src/remote/protocol.cpp:294)
		if (i>=MAX_CNCT_VERSIONS) {
			continue;
		}

		// skip versions we can't speak
		if (protoversion!=PROTOCOL_VERSION10 &&
			(protoversion<PROTOCOL_VERSION11 ||
			protoversion>maxprotocolversion)) {
			continue;
		}

		// skip architectures we can't speak
		// (anything but arch_generic tells the client that our byte
		// order and alignment match its own, and takes it off of xdr)
		if (protoarchtype!=arch_generic) {
			continue;
		}

		// skip anything the client prefers less than what we have
		// (>= rather than >, so the last of equal weights wins,
		// matching firebird)
		if (accepted && prefwt<acptweight) {
			continue;
		}

		accepted=true;
		acptweight=prefwt;
		acptversion=protoversion;
		acptarchtype=protoarchtype;

		// take the client's maximum type, capped at ours
		// (the minimum type is read and ignored, as firebird does)
		acpttype=maxtype&ptype_MASK;
		if (acpttype>MAX_PROTOCOL_TYPE) {
			acpttype=MAX_PROTOCOL_TYPE;
		}

		// pflag_compress is deliberately not carried over from
		// maxtype.  The client turns on zlib framing for every byte
		// after the accept the moment it sees that bit.
	}

	debugEnd();

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_accept
	// 	int32_t		p_acpt_version
	// 	int32_t		p_acpt_architecture
	// 	int32_t		p_acpt_type
	// }
	//
	// or, at protocol 13 and up, op_accept_data - see acceptData()
	//
	// or, if nothing offered could be spoken:
	//
	// data {
	// 	int32_t		op_reject
	// }

	debugStart("connect response");

	uint32_t	byteswritten=0;

	// reject if nothing offered could be spoken
	if (!accepted) {
		opcode=op_reject;
		if (!writeInt(opcode,"reject op code",&byteswritten)) {
			return false;
		}
		debugOpCode("reject op code",opcode);
		debugEnd();
		clientsock->flushWriteBuffer(-1,-1);
		return false;
	}

	protocolversion=acptversion;

	// protocol 13 and up answer op_accept_data and drive the auth plugin
	// handshake
	if (protocolversion>=PROTOCOL_VERSION13) {
		if (!acceptData(acptversion,acptarchtype,
					acpttype,&byteswritten)) {
			return false;
		}
		debugEnd();
		clientsock->flushWriteBuffer(-1,-1);
		return true;
	}

	opcode=op_accept;
	if (!writeInt(opcode,"accept op code",&byteswritten)) {
		return false;
	}
	debugOpCode("accept op code",opcode);

	if (!writeInt(acptversion,"protocol version",&byteswritten)) {
		return false;
	}
	debugProtocolVersion(acptversion);

	if (!writeInt(acptarchtype,"arch type",&byteswritten)) {
		return false;
	}
	debugArchType(acptarchtype);

	if (!writeInt(acpttype,"accept type",&byteswritten)) {
		return false;
	}
	debugProtocolType("accept type",acpttype);

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

// The user id block in the connect request is a sequence of tag/length/value
// items, each length a single byte.  Protocol 13 and up carry the first round
// of the auth plugin handshake in it - see ServerAuth::ServerAuth(),
// server.cpp:568-627.
void sqlrprotocol_firebird::parseUserId(const byte_t *userid,
						uint32_t useridlen) {

	// how much room the specific data needs
	// (an item's value is capped at 255 bytes, so a longer plugin message
	// arrives split across several items, each prefixed by its sequence
	// number - see getMultiPartConnectParameter(), server.cpp:514-558)
	uint32_t	datasize=0;

	const byte_t	*ptr=userid;
	const byte_t	*endptr=userid+useridlen;
	while ((size_t)(endptr-ptr)>1) {

		// get the tag and the value length
		byte_t	tag=0;
		read(ptr,&tag,&ptr);
		byte_t	valuelen=0;
		read(ptr,&valuelen,&ptr);

		// bail if the value runs past the end of the buffer
		if ((size_t)valuelen>(size_t)(endptr-ptr)) {
			break;
		}

		// step over the value
		const byte_t	*value=ptr;
		ptr+=valuelen;

		switch (tag) {
			case CNCT_login:
				readStringFromBuffer(value,valuelen,
							"login",&clientlogin);
				break;
			case CNCT_plugin_name:
				readStringFromBuffer(value,valuelen,
							"plugin name",
							&clientplugin);
				break;
			case CNCT_plugin_list:
				readStringFromBuffer(value,valuelen,
							"plugin list",
							&clientpluginlist);
				break;
			case CNCT_specific_data:
				if (valuelen>1) {
					uint32_t	end=
						value[0]*
						FIREBIRD_CNCT_SEGMENT_SIZE+
						valuelen-1;
					if (end>datasize) {
						datasize=end;
					}
				}
				break;
			default:
				break;
		}
	}

	if (!datasize) {
		return;
	}

	// reassemble the specific data
	// (a missing chunk leaves a run of nulls, which every plugin's data -
	// hex text for srp, a des hash for legacy_auth - fails to parse, so
	// there's nothing more to check for here)
	delete[] authclientdata;
	authclientdata=new char[datasize+1];
	bytestring::zero(authclientdata,datasize+1);

	ptr=userid;
	while ((size_t)(endptr-ptr)>1) {

		byte_t	tag=0;
		read(ptr,&tag,&ptr);
		byte_t	valuelen=0;
		read(ptr,&valuelen,&ptr);
		if ((size_t)valuelen>(size_t)(endptr-ptr)) {
			break;
		}
		const byte_t	*value=ptr;
		ptr+=valuelen;

		if (tag==CNCT_specific_data && valuelen>1) {
			bytestring::copy(authclientdata+
					value[0]*FIREBIRD_CNCT_SEGMENT_SIZE,
					value+1,valuelen-1);
		}
	}

	if (getDebug()) {
		stdoutput.printf("	specific data: %s\n",authclientdata);
	}
}

// Whether the client's CNCT_plugin_list names "plugin".  The list is
// comma-separated, and firebird's ParsedList tolerates spaces around the
// names.
bool sqlrprotocol_firebird::clientSupportsPlugin(const char *plugin) {

	if (charstring::isNullOrEmpty(clientpluginlist)) {
		return false;
	}

	size_t		pluginlen=charstring::getLength(plugin);
	const char	*p=clientpluginlist;
	while (p && *p) {

		// step over leading whitespace
		while (*p==' ' || *p=='\t') {
			p++;
		}

		// find the end of this name, ignoring trailing whitespace
		const char	*comma=charstring::findFirst(p,',');
		size_t		len=(comma)?(size_t)(comma-p):
						charstring::getLength(p);
		while (len && (p[len-1]==' ' || p[len-1]=='\t')) {
			len--;
		}

		if (len==pluginlen && !charstring::compare(p,plugin,len)) {
			return true;
		}

		p=(comma)?comma+1:NULL;
	}
	return false;
}

void sqlrprotocol_firebird::setAuthMethodFromPlugin(const char *plugin) {
	if (!charstring::compare(plugin,FIREBIRD_PLUGIN_SRP256)) {
		authmethod=FIREBIRD_SRP256;
	} else if (!charstring::compare(plugin,FIREBIRD_PLUGIN_SRP)) {
		authmethod=FIREBIRD_SRP;
	} else if (!charstring::compare(plugin,FIREBIRD_PLUGIN_LEGACY)) {
		authmethod=FIREBIRD_LEGACY;
	} else {
		authmethod=NULL;
	}
}

// SrpServer::authenticate() packs each half of its answer as a 2-byte
// little-endian length and that many bytes of hex text - SrpServer.cpp:
// 330-338.
static void appendSrpData(bytebuffer *data, const char *value) {
	size_t	len=charstring::getLength(value);
	data->append((char)(len&0xff));
	data->append((char)((len>>8)&0xff));
	data->append(value,len);
}

// Runs the server's half of the srp exchange's first round and formats the
// answer the way SrpServer::authenticate() does - SrpServer.cpp:328-340:
//
//	int16_t		salt length, little endian
//	char[]		salt, as hex text
//	int16_t		server public key length, little endian
//	char[]		server public key B, as hex text
bool sqlrprotocol_firebird::srpChallenge(bytebuffer *data) {

	// the client's public key A came in as hex text
	if (charstring::isNullOrEmpty(authclientdata)) {
		return false;
	}

	// The ephemeral private key b belongs to the session rather than to
	// the auth module.  challenge() keeps no state, so handing the same b
	// back at verify time is what reproduces B, and with it the session
	// key.  See the contract at the top of
	// src/auths/firebird_connectstrings.cpp.
	byte_t	privatekey[FIREBIRD_SRP_PRIVATE_KEY_SIZE];
	csprng	rng;
	if (!rng.generateBytes(privatekey,sizeof(privatekey))) {
		return false;
	}
	delete[] srpserverprivatekey;
	srpserverprivatekey=charstring::hexEncode(privatekey,
						sizeof(privatekey));
	bytestring::zero(privatekey,sizeof(privatekey));

	stringbuffer	extra;
	extra.append("clientpublickey=")->append(authclientdata);
	extra.append(";serverprivatekey=")->append(srpserverprivatekey);

	sqlrfirebirdcredentials	cred;
	cred.setUser(username);
	cred.setMethod(authmethod);
	cred.setExtra(extra.getString());

	// A false return means no auth module knows the user, or has its
	// password under a one-way encryption, or supports the method.
	// Firebird's srp plugin answers AUTH_CONTINUE in the same situation
	// and the server moves on to the next plugin - SrpServer.cpp:400-410
	// and server.cpp:2166-2176 - so the caller does that here too.
	stringbuffer	challenge;
	if (!cont->challenge(&cred,&challenge)) {
		if (getDebug()) {
			stdoutput.write("	srp challenge failed\n");
		}
		return false;
	}

	parameterstring	p;
	p.parse(challenge.getString());

	delete[] srpsalt;
	srpsalt=charstring::duplicate(p.getValue("salt"));
	const char	*serverpublickey=p.getValue("serverpublickey");
	if (charstring::isNullOrEmpty(srpsalt) ||
			charstring::isNullOrEmpty(serverpublickey)) {
		return false;
	}

	appendSrpData(data,srpsalt);
	appendSrpData(data,serverpublickey);

	if (getDebug()) {
		stdoutput.printf("	srp salt: %s\n",srpsalt);
		stdoutput.printf("	srp server public key: %s\n",
							serverpublickey);
	}

	return true;
}

// Picks the plugin to continue the handshake with, and builds whatever data
// goes back with it.  Mirrors accept_connection(), server.cpp:2126-2190.
bool sqlrprotocol_firebird::selectAuthPlugin(bytebuffer *data) {

	delete[] authplugin;
	authplugin=NULL;
	authmethod=NULL;

	// CNCT_login is the same value the attach dpb sends as
	// isc_dpb_user_name, and the plugins need it now rather than then -
	// srp hashes it into the verifier and into the proof
	// (only 13 and up gets here, so nothing below it changes)
	if (!charstring::isNullOrEmpty(clientlogin)) {
		delete[] username;
		username=charstring::duplicate(clientlogin);
	}

	bool	srpfailed=false;

	// the client led with a plugin we have...
	setAuthMethodFromPlugin(clientplugin);
	if (authmethod) {

		authplugin=charstring::duplicate(clientplugin);

		// legacy_auth's first message is the des hash of the
		// password, and the client sends that again in the attach
		// dpb, so there's nothing to answer with here
		if (!charstring::compare(authmethod,FIREBIRD_LEGACY)) {
			return true;
		}

		if (srpChallenge(data)) {
			return true;
		}

		// the challenge couldn't be built, so fall through and offer
		// a plugin that doesn't need it
		srpfailed=true;
		data->clear();
		delete[] authplugin;
		authplugin=NULL;
		authmethod=NULL;
	}

	// Otherwise, name the best plugin we have that the client also has,
	// and let it start over with that one.  An empty p_acpt_data next to
	// a plugin name is what tells the client to do that -
	// server.cpp:2177-2181.
	//
	// The srp plugins are skipped when the challenge above already failed,
	// because both of them derive the verifier from the same configured
	// password and would fail for the same reason.  Firebird would walk
	// its whole list here; there is nothing to be gained from the extra
	// round trips.
	static const char	*plugins[]={
		FIREBIRD_PLUGIN_SRP256,
		FIREBIRD_PLUGIN_SRP,
		FIREBIRD_PLUGIN_LEGACY,
		NULL
	};
	for (const char * const *plugin=plugins; *plugin; plugin++) {
		if (srpfailed && charstring::compare(*plugin,
						FIREBIRD_PLUGIN_LEGACY)) {
			continue;
		}
		if (clientSupportsPlugin(*plugin)) {
			authplugin=charstring::duplicate(*plugin);
			setAuthMethodFromPlugin(authplugin);
			return true;
		}
	}

	// nothing in common - the accept goes out with no plugin at all, and
	// the attach below fails the login
	return false;
}

bool sqlrprotocol_firebird::acceptData(uint32_t acptversion,
					uint32_t acptarchtype,
					uint32_t acpttype,
					uint32_t *byteswritten) {

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_accept_data
	// 	int32_t		p_acpt_version
	// 	int32_t		p_acpt_architecture
	// 	int32_t		p_acpt_type
	// 	int32_t		p_acpt_data length
	// 	byte_t[]	p_acpt_data
	// 	int32_t		p_acpt_plugin length
	// 	char[]		p_acpt_plugin
	// 	int32_t		p_acpt_authenticated
	// 	int32_t		p_acpt_keys length
	// 	byte_t[]	p_acpt_keys
	// }
	//
	// (p_acpd, protocol.h - the field order is the xdr order at
	// protocol.cpp:365-376.  p_acpt_authenticated is a USHORT, and xdr
	// puts a short on the wire as 4 bytes, the same as every other length
	// and op code here.)
	//
	// op_cond_accept carries exactly the same fields, and firebird uses it
	// in place of op_accept_data only when wire encryption is being
	// negotiated - the AUTH_COND_ACCEPT path at server.cpp:2084-2098 and
	// 764-772.  This module doesn't do wire encryption, so it always sends
	// op_accept_data.

	bytebuffer	data;
	selectAuthPlugin(&data);

	// an empty buffer's getBuffer() and a null plugin name are both
	// written as a zero length and no bytes, so they need something
	// non-null to point at
	static const byte_t	empty[1]={0};

	opcode=op_accept_data;
	if (!writeInt(opcode,"accept data op code",byteswritten)) {
		return false;
	}
	debugOpCode("accept data op code",opcode);

	if (!writeInt(acptversion,"protocol version",byteswritten)) {
		return false;
	}
	debugProtocolVersion(acptversion);

	if (!writeInt(acptarchtype,"arch type",byteswritten)) {
		return false;
	}
	debugArchType(acptarchtype);

	if (!writeInt(acpttype,"accept type",byteswritten)) {
		return false;
	}
	debugProtocolType("accept type",acpttype);

	uint32_t	datalen=(uint32_t)data.getSize();
	if (!writeBuffer((datalen)?data.getBuffer():empty,datalen,
					"plugin data",byteswritten)) {
		return false;
	}

	uint32_t	pluginlen=(uint32_t)charstring::getLength(authplugin);
	if (!writeBuffer((pluginlen)?(const byte_t *)authplugin:empty,
				pluginlen,"plugin name",byteswritten)) {
		return false;
	}

	// not authenticated yet - the attach below runs the auth modules
	// (a real firebird can finish a login inside the accept, but only for
	// a plugin that needs no round trip at all, and none of the three
	// here is one)
	if (!writeInt(0,"authenticated",byteswritten)) {
		return false;
	}

	// no keys - the module offers no wire encryption, and an empty list
	// is what firebird sends when it has none either
	if (!writeBuffer(empty,0,"keys",byteswritten)) {
		return false;
	}

	return true;
}

// The second and later rounds of the plugin handshake, when the client's
// first message didn't arrive with the connect request.  The server sends
// op_cont_auth and the client answers with one - ServerAuth::authenticate()
// at server.cpp:773-780 and continue_authentication() at
// server.cpp:5446-5452.
//
// Only the srp plugins ever get here, and only when the client had to start
// them over on the plugin the accept named.  In the ordinary case the client
// leads with a plugin the module has, its public key rides in
// CNCT_specific_data, and the whole exchange is done by the time the attach
// arrives.
bool sqlrprotocol_firebird::continueAuthentication() {

	// nothing to continue - either the plugin needs no more rounds, or
	// the challenge already ran during the connect
	if ((charstring::compare(authmethod,FIREBIRD_SRP) &&
		charstring::compare(authmethod,FIREBIRD_SRP256)) || srpsalt) {
		return true;
	}

	debugStart("continue auth");

	// what arrived in the attach dpb is the client's public key, not its
	// proof, so answer the challenge for it
	delete[] authclientdata;
	authclientdata=password;
	password=NULL;

	bytebuffer	data;
	if (!srpChallenge(&data)) {
		debugEnd();
		return false;
	}

	// request/response packet data structure:
	//
	// data {
	// 	int32_t		op_cont_auth
	// 	int32_t		p_data length
	// 	byte_t[]	p_data
	// 	int32_t		p_name length
	// 	char[]		p_name
	// 	int32_t		p_list length
	// 	char[]		p_list
	// 	int32_t		p_keys length
	// 	byte_t[]	p_keys
	// }
	//
	// (P_AUTH_CONT, protocol.h - the field order is the xdr order at
	// protocol.cpp:813-822)

	uint32_t	byteswritten=0;

	opcode=op_cont_auth;
	if (!writeInt(opcode,"cont auth op code",&byteswritten)) {
		debugEnd();
		return false;
	}
	debugOpCode("cont auth op code",opcode);

	static const byte_t	empty[1]={0};

	uint32_t	datalen=(uint32_t)data.getSize();
	uint32_t	pluginlen=(uint32_t)charstring::getLength(authplugin);
	if (!writeBuffer((datalen)?data.getBuffer():empty,datalen,
				"plugin data",&byteswritten) ||
		!writeBuffer((pluginlen)?(const byte_t *)authplugin:empty,
				pluginlen,"plugin name",&byteswritten) ||
		!writeBuffer((const byte_t *)FIREBIRD_PLUGIN_LIST,
				(uint32_t)charstring::getLength(
						FIREBIRD_PLUGIN_LIST),
				"plugin list",&byteswritten) ||
		!writeBuffer(empty,0,"keys",&byteswritten)) {
		debugEnd();
		return false;
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	// get the client's answer
	debugStart("cont auth");

	uint32_t	bytesread=0;

	if (!readInt(&opcode,"cont auth op code",op_cont_auth,&bytesread)) {
		debugEnd();
		return false;
	}
	debugOpCode("cont auth op code",opcode);

	// the proof replaces whatever the attach dpb had
	delete[] password;
	if (!readString(&password,"plugin data",&bytesread)) {
		debugEnd();
		return false;
	}

	// the plugin name, plugin list and keys come back too, but the
	// exchange is already committed to a plugin, so they're read and
	// discarded
	char	*discard=NULL;
	if (!readString(&discard,"plugin name",&bytesread)) {
		debugEnd();
		return false;
	}
	delete[] discard;
	if (!readString(&discard,"plugin list",&bytesread)) {
		debugEnd();
		return false;
	}
	delete[] discard;
	if (!readString(&discard,"keys",&bytesread)) {
		debugEnd();
		return false;
	}
	delete[] discard;

	debugEnd();

	return true;
}

bool sqlrprotocol_firebird::attach() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		op_attach
	// 	int32_t		db object id
	// 	int32_t		db length
	// 	char[]		db path or alias
	// 	int32_t		db parameter buffer length
	// 	byte_t[]	db parameter buffer
	// 	...
	// }

	debugStart("attach");

	uint32_t	bytesread=0;

	// get op_attach
	if (!readInt(&opcode,"attach op code",&bytesread)) {
		return false;
	}
	debugOpCode("attach op code",opcode);

	// a create, drop or service-manager op arrives here instead of
	// op_attach, ahead of the session loop that would otherwise see it
	if (opcode!=op_attach) {
		debugEnd();
		return sendNotImplementedError();
	}

	// get db object id
	uint32_t	dbobjectid=0;
	if (!readInt(&dbobjectid,"db object id",&bytesread)) {
		return false;
	}

	// get db (override db from connect)
	delete[] db;
	if (!readString(&db,"db",&bytesread)) {
		return false;
	}

	// get db parameters buffer
	uint32_t	dpblen;
	byte_t		*dpb;
	if (!readBuffer(&dpb,&dpblen,"db param buffer",&bytesread)) {
		return false;
	}

	// the plugin the dpb says its auth data belongs to, if any
	char	*dpbplugin=NULL;

	// whether the dpb carried plugin auth data, which wins over the
	// password items the way ServerAuth::ServerAuth() prefers it -
	// server.cpp:597-628
	bool	haveauthdata=false;

	// process db parameters buffer...
	const byte_t	*dpbptr=dpb;
	const byte_t	*dpbendptr=dpb+dpblen;

	// get the dpb version
	byte_t	dpbversion=0;
	if (dpbptr!=dpbendptr) {
		read(dpbptr,&dpbversion,&dpbptr);
		debugDpbVersion(dpbversion);
	}

	// the version byte selects the item encoding - version1 gives each
	// item a 1-byte value length, version2 a 4-byte little-endian one
	// (any other version leaves the framing unknown, so the walk below is
	// skipped rather than guessed at)
	uint32_t	dpblensize=0;
	if (dpbversion==isc_dpb_version1) {
		dpblensize=1;
	} else if (dpbversion==isc_dpb_version2) {
		dpblensize=4;
	} else {
		dpbptr=dpbendptr;
	}

	// get each parameter...
	// (every dpb item is a parameter byte, a value length, and that many
	// value bytes, without exception, so an item that no case below
	// consumes is skipped by its length)
	while ((size_t)(dpbendptr-dpbptr)>dpblensize) {

		// get the parameter
		byte_t	dpbparam;
		read(dpbptr,&dpbparam,&dpbptr);
		debugDpbParam(dpbparam);

		// get the value length
		uint32_t	dpbvaluelen=0;
		if (dpblensize==4) {
			readLE(dpbptr,&dpbvaluelen,&dpbptr);
		} else {
			byte_t	len;
			read(dpbptr,&len,&dpbptr);
			dpbvaluelen=len;
		}
		if (getDebug()) {
			stdoutput.printf("	dpb value length: %u\n",
							dpbvaluelen);
		}

		// bail if the value runs past the end of the buffer
		if (dpbvaluelen>(size_t)(dpbendptr-dpbptr)) {
			if (getDebug()) {
				stdoutput.write("	dpb value runs past "
						"the end of the buffer\n");
			}
			break;
		}

		// step over the value
		const byte_t	*dpbvalue=dpbptr;
		dpbptr+=dpbvaluelen;

		// process the parameter...
		switch (dpbparam) {
			case isc_dpb_cdd_pathname:
				// FIXME: do something...
				break;

			case isc_dpb_allocation:
				// FIXME: do something...
				break;

			case isc_dpb_journal:
				// FIXME: do something...
				break;

			case isc_dpb_page_size:
				// FIXME: do something...
				break;

			case isc_dpb_num_buffers:
				// FIXME: do something...
				break;

			case isc_dpb_buffer_length:
				// FIXME: do something...
				break;

			case isc_dpb_debug:
				// FIXME: do something...
				break;

			case isc_dpb_garbage_collect:
				// FIXME: do something...
				break;

			case isc_dpb_verify:
				// FIXME: do something...
				break;

			case isc_dpb_sweep:
				// FIXME: do something...
				break;

			case isc_dpb_enable_journal:
				// FIXME: do something...
				break;

			case isc_dpb_disable_journal:
				// FIXME: do something...
				break;

			case isc_dpb_dbkey_scope:
				// FIXME: do something...
				break;

			case isc_dpb_number_of_users:
				// FIXME: do something...
				break;

			case isc_dpb_trace:
				// FIXME: do something...
				break;

			case isc_dpb_no_garbage_collect:
				// FIXME: do something...
				break;

			case isc_dpb_damaged:
				// FIXME: do something...
				break;

			case isc_dpb_license:
				// FIXME: do something...
				break;

			case isc_dpb_sys_user_name:
				// FIXME: do something...
				break;

			case isc_dpb_encrypt_key:
				// FIXME: do something...
				break;

			case isc_dpb_activate_shadow:
				// FIXME: do something...
				break;

			case isc_dpb_sweep_interval:
				// FIXME: do something...
				break;

			case isc_dpb_delete_shadow:
				// FIXME: do something...
				break;

			case isc_dpb_force_write:
				// FIXME: do something...
				break;

			case isc_dpb_begin_log:
				// FIXME: do something...
				break;

			case isc_dpb_quit_log:
				// FIXME: do something...
				break;

			case isc_dpb_no_reserve:
				// FIXME: do something...
				break;

			case isc_dpb_user_name:
				// at 13 and up, username is already set from
				// CNCT_login, and an auth plugin's verifier and
				// proof were built from that value - overwriting
				// it here would make the two disagree and every
				// srp login would fail its proof check
				if (protocolversion<PROTOCOL_VERSION13) {
					readStringFromBuffer(dpbvalue,dpbvaluelen,
								"user name",&username);
				} else {
					char	*discard=NULL;
					readStringFromBuffer(dpbvalue,dpbvaluelen,
								"user name",&discard);
					delete[] discard;
				}
				break;

			// isc_dpb_password is the password itself and
			// isc_dpb_password_enc firebird's legacy_auth hash
			// of it
			// (fbclient rewrites the former into the latter, so
			// only a client that builds its own dpb sends the
			// password)
			case isc_dpb_password:
				if (haveauthdata) {
					break;
				}
				readStringFromBuffer(dpbvalue,dpbvaluelen,
							"password",&password);
				authmethod=FIREBIRD_CLEARTEXT;
				break;

			case isc_dpb_password_enc:
				if (haveauthdata) {
					break;
				}
				readStringFromBuffer(dpbvalue,dpbvaluelen,
							"password",&password);
				authmethod=FIREBIRD_LEGACY;
				break;


			case isc_dpb_sys_user_name_enc:
				// FIXME: do something...
				break;

			case isc_dpb_interp:
				// FIXME: do something...
				break;

			case isc_dpb_online_dump:
				// FIXME: do something...
				break;

			case isc_dpb_old_file_size:
				// FIXME: do something...
				break;

			case isc_dpb_old_num_files:
				// FIXME: do something...
				break;

			case isc_dpb_old_file:
				// FIXME: do something...
				break;

			case isc_dpb_old_start_page:
				// FIXME: do something...
				break;

			case isc_dpb_old_start_seqno:
				// FIXME: do something...
				break;

			case isc_dpb_old_start_file:
				// FIXME: do something...
				break;

			case isc_dpb_drop_walfile:
				// FIXME: do something...
				break;

			case isc_dpb_old_dump_id:
				// FIXME: do something...
				break;

			case isc_dpb_wal_backup_dir:
				// FIXME: do something...
				break;

			case isc_dpb_wal_chkptlen:
				// FIXME: do something...
				break;

			case isc_dpb_wal_numbufs:
				// FIXME: do something...
				break;

			case isc_dpb_wal_bufsize:
				// FIXME: do something...
				break;

			case isc_dpb_wal_grp_cmt_wait:
				// FIXME: do something...
				break;

			case isc_dpb_lc_messages:
				// FIXME: do something...
				break;

			case isc_dpb_lc_ctype:
				// FIXME: do something...
				break;

			case isc_dpb_cache_manager:
				// FIXME: do something...
				break;

			case isc_dpb_shutdown:
				// FIXME: do something...
				break;

			case isc_dpb_online:
				// FIXME: do something...
				break;

			case isc_dpb_shutdown_delay:
				// FIXME: do something...
				break;

			case isc_dpb_reserved:
				// FIXME: do something...
				break;

			case isc_dpb_overwrite:
				// FIXME: do something...
				break;

			case isc_dpb_sec_attach:
				// FIXME: do something...
				break;

			case isc_dpb_disable_wal:
				// FIXME: do something...
				break;

			case isc_dpb_connect_timeout:
				// FIXME: do something...
				break;

			case isc_dpb_dummy_packet_interval:
				// FIXME: do something...
				break;

			case isc_dpb_gbak_attach:
				// FIXME: do something...
				break;

			case isc_dpb_sql_role_name:
				// FIXME: do something...
				break;

			case isc_dpb_set_page_buffers:
				// FIXME: do something...
				break;

			case isc_dpb_working_directory:
				readStringFromBuffer(dpbvalue,dpbvaluelen,
						"working directory",&wd);
				break;

			case isc_dpb_sql_dialect:
				// FIXME: do something...
				break;

			case isc_dpb_set_db_readonly:
				// FIXME: do something...
				break;

			case isc_dpb_set_db_sql_dialect:
				// FIXME: do something...
				break;

			case isc_dpb_gfix_attach:
				// FIXME: do something...
				break;

			case isc_dpb_gstat_attach:
				// FIXME: do something...
				break;

			case isc_dpb_set_db_charset:
				// FIXME: do something...
				break;

			case isc_dpb_gsec_attach:
				// FIXME: do something...
				break;

			case isc_dpb_address_path:
				// FIXME: do something...
				break;

			case isc_dpb_process_id:
				// FIXME: do something...
				break;

			case isc_dpb_no_db_triggers:
				// FIXME: do something...
				break;

			case isc_dpb_trusted_auth:
				// FIXME: do something...
				break;

			case isc_dpb_process_name:
				// FIXME: do something...
				break;

			case isc_dpb_trusted_role:
				// FIXME: do something...
				break;

			case isc_dpb_org_filename:
				// FIXME: do something...
				break;

			case isc_dpb_utf8_filename:
				// FIXME: do something...
				break;

			case isc_dpb_ext_call_depth:
				// FIXME: do something...
				break;

			case isc_dpb_auth_block:
				// FIXME: do something...
				break;

			case isc_dpb_client_version:
				// FIXME: do something...
				break;

			case isc_dpb_remote_protocol:
				// FIXME: do something...
				break;

			case isc_dpb_host_name:
				// FIXME: do something...
				break;

			case isc_dpb_os_user:
				// FIXME: do something...
				break;

			// the auth plugin handshake continues here at
			// protocol 13 and up - the plugin's next message is
			// the specific auth data, and the plugin it belongs
			// to is the plugin name
			// (below 13 no client sends these, and the module
			// ignores them rather than letting one steer the
			// authentication method)
			case isc_dpb_specific_auth_data:
				if (protocolversion>=PROTOCOL_VERSION13) {
					readStringFromBuffer(dpbvalue,
							dpbvaluelen,
							"specific auth data",
							&password);
					haveauthdata=true;
				}
				break;

			case isc_dpb_auth_plugin_list:
				// FIXME: do something...
				break;

			case isc_dpb_auth_plugin_name:
				if (protocolversion>=PROTOCOL_VERSION13) {
					readStringFromBuffer(dpbvalue,
							dpbvaluelen,
							"auth plugin name",
							&dpbplugin);
				}
				break;

			case isc_dpb_config:
				// FIXME: do something...
				break;

			case isc_dpb_nolinger:
				// FIXME: do something...
				break;

			case isc_dpb_reset_icu:
				// FIXME: do something...
				break;

			case isc_dpb_map_attach:
				// FIXME: do something...
				break;

			case isc_dpb_session_time_zone:
				// FIXME: do something...
				break;

			case isc_dpb_set_db_replica:
				// FIXME: do something...
				break;

			case isc_dpb_set_bind:
				// FIXME: do something...
				break;

			case isc_dpb_decfloat_round:
				// FIXME: do something...
				break;

			case isc_dpb_decfloat_traps:
				// FIXME: do something...
				break;

			case isc_dpb_clear_map:
				// FIXME: do something...
				break;

			default:
				// unknown items are skipped by their length
				break;
		}
	}

	debugEnd();

	// clean up
	delete[] dpb;

	// at protocol 13 and up the plugin the dpb names wins over whichever
	// dpb item the password came out of
	// (a client that fell back to a different plugin than the accept
	// asked for names the one it actually used)
	if (protocolversion>=PROTOCOL_VERSION13 && dpbplugin) {
		if (charstring::compare(dpbplugin,authplugin)) {
			delete[] authplugin;
			authplugin=charstring::duplicate(dpbplugin);
			// a plugin switch invalidates the challenge the
			// accept sent
			delete[] srpsalt;
			srpsalt=NULL;
		}
		setAuthMethodFromPlugin(dpbplugin);
	}
	delete[] dpbplugin;

	// finish the plugin handshake, if it needs more rounds
	if (protocolversion>=PROTOCOL_VERSION13 && !continueAuthentication()) {
		errorStatusVector(isc_login);
		genericResponse("attach failure response",
					0,0,
					NULL,0,
					statusvector,statusvectorstr,
					statusvectorlen);
		return false;
	}

	// authenticate
	// (the credentials came out of the dpb above)
	if (!authenticate()) {
		return false;
	}

	// validate the database
	// (after authenticating, not before - a real firebird checks the
	// credentials first and the database file second, and answers a bad
	// password even when the file is missing too)
	if (!validateDatabase()) {
		return false;
	}

	// FIXME: object handle should be the database handle ???
	// FIXME: no idea what the database handle is
	uint32_t	objecthandle=0;

	// FIXME: no idea what the object id is
	uint32_t	objectid=0;

	// status vector...
	successStatusVector();

	return genericResponse("attach response",
				objecthandle,((uint64_t)objectid)<<32,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

void sqlrprotocol_firebird::successStatusVector() {
	bytestring::zero(statusvector,sizeof(statusvector));
	bytestring::zero(statusvectorstr,sizeof(statusvectorstr));
	// interbase error...
	statusvector[0]=isc_arg_gds;
	// no error...
	statusvector[1]=0;
	// end of vector...
	// (a client reads elements until it sees isc_arg_end, so the
	// terminator isn't optional)
	statusvector[2]=isc_arg_end;
	statusvectorlen=3;
}

void sqlrprotocol_firebird::errorStatusVector(uint32_t gdscode) {
	bytestring::zero(statusvector,sizeof(statusvector));
	bytestring::zero(statusvectorstr,sizeof(statusvectorstr));
	// interbase error...
	statusvector[0]=isc_arg_gds;
	// the error...
	statusvector[1]=gdscode;
	// end of vector...
	statusvector[2]=isc_arg_end;
	statusvectorlen=3;
}

void sqlrprotocol_firebird::openErrorStatusVector(const char *file) {
	bytestring::zero(statusvector,sizeof(statusvector));
	bytestring::zero(statusvectorstr,sizeof(statusvectorstr));
	// the outer error...
	statusvector[0]=isc_arg_gds;
	statusvector[1]=isc_io_error;
	// what failed, and what it failed on...
	statusvector[2]=isc_arg_string;
	statusvectorstr[3]="open";
	statusvector[4]=isc_arg_string;
	statusvectorstr[5]=file;
	// the inner error...
	statusvector[6]=isc_arg_gds;
	statusvector[7]=isc_io_open_err;
	// the errno text, already rendered
	// (which is what isc_arg_interpreted means, as opposed to
	// isc_arg_string, which the client renders itself.  this text and the
	// "open" above are a real firebird's own, captured off the wire and
	// byte-for-byte in 2.5, 3.0 and 4.0.)
	statusvector[8]=isc_arg_interpreted;
	statusvectorstr[9]="No such file or directory";
	// end of vector...
	statusvector[10]=isc_arg_end;
	statusvectorlen=11;
}

bool sqlrprotocol_firebird::genericResponse(const char *title,
						uint32_t objecthandle,
						uint64_t blobid,
						const byte_t *buffer,
						uint32_t bufferlen,
						uint32_t *sv,
						const char **svstr,
						uint8_t svlen) {

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_response
	// 	int32_t		object handle
	// 	int32_t		object id (blob id, high word)
	// 	int32_t		blob id, low word
	// 	int32_t		buffer length
	// 	byte_t[]	buffer
	// 	byte_t[]	buffer padding
	// 	int32_t[]	status vector
	// }

	debugStart(title);

	uint32_t	byteswritten=0;

	// write the opcode
	opcode=op_response;
	if (!writeInt(opcode,"response op code",&byteswritten)) {
		return false;
	}
	debugOpCode("response op code",opcode);

	// write the object handle
	if (!writeInt(objecthandle,"object handle",&byteswritten)) {
		return false;
	}

	// write the blob id
	// (an 8-byte blob id is always on the wire, even in a response that
	// has nothing to do with blobs, and its high word is what firebird
	// calls the response's object id)
	if (!writeInt((uint32_t)(blobid>>32),
				"object id",&byteswritten) ||
		!writeInt((uint32_t)(blobid&0xffffffff),
				"blob id low word",&byteswritten)) {
		return false;
	}

	// write the buffer
	if (!writeBuffer(buffer,bufferlen,"buffer",&byteswritten)) {
		return false;
	}

	// write the status vector
	if (!writeStatusVector(sv,svstr,svlen,&byteswritten)) {
		return false;
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

bool sqlrprotocol_firebird::authenticate() {

	// the srp methods verify a proof rather than a password, and need the
	// inputs of the round that produced it back
	// (see the contract at the top of
	// src/auths/firebird_connectstrings.cpp)
	stringbuffer	extra;
	if (!charstring::compare(authmethod,FIREBIRD_SRP) ||
			!charstring::compare(authmethod,FIREBIRD_SRP256)) {
		extra.append("clientpublickey=")->append(authclientdata);
		extra.append(";serverprivatekey=")->
					append(srpserverprivatekey);
		extra.append(";salt=")->append(srpsalt);
	}

	// build auth credentials
	sqlrfirebirdcredentials	cred;
	cred.setUser(username);
	cred.setPassword(password);
	cred.setPasswordSize(charstring::getLength(password));
	cred.setMethod(authmethod);
	cred.setExtra(extra.getString());

	// authenticate
	bool	retval=cont->auth(&cred);

	if (getDebug()) {
		debugStart("authenticate");
		stdoutput.printf("	auth %s\n",(retval)?"success":"failed");
		debugEnd();
	}

	// success
	if (retval) {
		return true;
	}

	// isc_login and no message text is what a real firebird sends - the
	// client renders the text from the code itself
	errorStatusVector(isc_login);

	// the response is sent, but the session still ends
	genericResponse("attach failure response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
	return false;
}

bool sqlrprotocol_firebird::validateDatabase() {

	// which database the instance fronts
	// ("db" with a "database" fallback, the way the firebird connection
	// module's handleConnectString() reads it)
	const char	*condb=cont->getConnectStringValue("db");
	if (charstring::isNullOrEmpty(condb)) {
		condb=cont->getConnectStringValue("database");
	}

	// nothing to compare
	if (charstring::isNullOrEmpty(condb) ||
			charstring::isNullOrEmpty(db)) {
		return true;
	}

	// step over any host prefix
	// (the client uses the "host:" or "host/port:" on the front of the
	// path to pick the server and strips it before sending, so it's in
	// the configured value but never on the wire.  Only one character
	// before the colon is a windows drive letter rather than a host, and
	// a value that starts with a slash is a path whose colon is its own.)
	const char	*colon=charstring::findFirst(condb,':');
	if (colon && colon-condb>1 &&
			condb[0]!='/' && condb[0]!='\\') {
		condb=colon+1;
	}

	// compare the strings exactly
	// (a real firebird resolves the path on its own filesystem and looks
	// up aliases, neither of which is available here)
	bool	valid=!charstring::compare(condb,db);

	if (getDebug()) {
		debugStart("validate database");
		stdoutput.printf("	requested: %s\n",db);
		stdoutput.printf("	available: %s\n",condb);
		stdoutput.printf("	database %s\n",
					(valid)?"valid":"invalid");
		debugEnd();
	}

	// success
	if (valid) {
		return true;
	}

	openErrorStatusVector(db);

	// the response is sent, but the session still ends
	genericResponse("attach failure response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
	return false;
}

bool sqlrprotocol_firebird::getOpCode() {

	debugStart("get op code");

	uint32_t	bytesread=0;
	
	if (!readInt(&opcode,"op code",&bytesread)) {
		return false;
	}
	debugOpCode("op code",opcode);
	debugEnd();
	return true;
}

bool sqlrprotocol_firebird::detach() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		db handle
	// }

	debugStart("detach");

	uint32_t	bytesread=0;

	uint32_t	clientdbhandle;
	if (!readInt(&clientdbhandle,"db handle",&bytesread)) {
		return false;
	}

	debugEnd();

	// end whatever is still open
	// (firebird rejects a detach with a live transaction, but SQL Relay's
	// session teardown rolls one back anyway)
	clearStatements();
	clearBlobs();
	clearArrays();
	if (intransaction) {
		cont->rollback();
		intransaction=false;
		trautocommit=false;
		trreadonly=false;
		trisolevel=0;
		trhandle=0;
	}

	successStatusVector();

	// stay in the loop - the client sends op_disconnect straight after
	// this and then closes the socket
	return genericResponse("detach response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::create() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::dropDatabase() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::appendInfoItem(byte_t item,
					const byte_t *value,
					uint16_t valuelen) {

	// the 4 is the item byte, the 2 length bytes, and 1 more held back
	// for the trailing isc_info_end
	// (a cluster that doesn't fit is replaced by a bare isc_info_truncated,
	// the reply gets no isc_info_end, and the status vector still says
	// success - see INF_put_item)
	if (respbuffer.getSize()+valuelen+4>=respbufferlen) {
		if (respbuffer.getSize()<respbufferlen) {
			write(&respbuffer,(byte_t)isc_info_truncated);
		}
		if (getDebug()) {
			stdoutput.printf("	truncated\n");
		}
		return false;
	}

	// item, 2 byte little-endian length, value
	write(&respbuffer,item);
	writeLE(&respbuffer,valuelen);
	if (valuelen) {
		write(&respbuffer,value,valuelen);
	}
	return true;
}

bool sqlrprotocol_firebird::appendInfoInt(byte_t item, uint32_t value) {
	byte_t	val[4];
	val[0]=(byte_t)(value&0xff);
	val[1]=(byte_t)((value>>8)&0xff);
	val[2]=(byte_t)((value>>16)&0xff);
	val[3]=(byte_t)((value>>24)&0xff);
	return appendInfoItem(item,val,sizeof(val));
}

bool sqlrprotocol_firebird::appendInfoByte(byte_t item, byte_t value) {
	return appendInfoItem(item,&value,1);
}

bool sqlrprotocol_firebird::appendInfoStrings(byte_t item,
					const char * const *values,
					byte_t valuecount) {

	// a count byte, then each string as a length byte and that many bytes
	byte_t		val[256];
	uint16_t	vallen=1;
	byte_t		valswritten=0;
	for (byte_t i=0; i<valuecount; i++) {
		size_t	len=charstring::getLength(values[i]);
		if (len>255) {
			len=255;
		}
		if (vallen+1+len>sizeof(val)) {
			break;
		}
		val[vallen++]=(byte_t)len;
		if (len) {
			bytestring::copy(val+vallen,values[i],len);
		}
		vallen+=len;
		valswritten++;
	}

	// the count has to be what actually fit, not what was asked for, or
	// a client counting entries walks off the end of the value
	val[0]=valswritten;

	return appendInfoItem(item,val,vallen);
}

bool sqlrprotocol_firebird::appendInfoCountedString(byte_t item,
						const char *value) {

	// a length byte, then that many bytes.  Unlike appendInfoStrings()
	// there's no count in front - a real server repeats the whole cluster
	// when there's more than one string.
	byte_t	val[256];
	size_t	len=charstring::getLength(value);
	if (len>sizeof(val)-1) {
		len=sizeof(val)-1;
	}
	val[0]=(byte_t)len;
	if (len) {
		bytestring::copy(val+1,value,len);
	}

	return appendInfoItem(item,val,(uint16_t)(len+1));
}

bool sqlrprotocol_firebird::appendInfoTimestamp(byte_t item,
						int32_t year,
						int32_t month,
						int32_t day,
						int32_t hour,
						int32_t minute,
						int32_t second) {

	// an ISC_TIMESTAMP - an ISC_DATE then an ISC_TIME, each little-endian
	uint32_t	date=encodeDate(year,month,day);
	uint32_t	time=encodeTime(hour,minute,second,0);
	byte_t		val[8];
	val[0]=(byte_t)(date&0xff);
	val[1]=(byte_t)((date>>8)&0xff);
	val[2]=(byte_t)((date>>16)&0xff);
	val[3]=(byte_t)((date>>24)&0xff);
	val[4]=(byte_t)(time&0xff);
	val[5]=(byte_t)((time>>8)&0xff);
	val[6]=(byte_t)((time>>16)&0xff);
	val[7]=(byte_t)((time>>24)&0xff);

	return appendInfoItem(item,val,sizeof(val));
}

bool sqlrprotocol_firebird::appendInfoError(byte_t item) {

	// the item the client asked for, then isc_infunk, little-endian
	byte_t	val[5];
	val[0]=item;
	val[1]=(byte_t)(isc_infunk&0xff);
	val[2]=(byte_t)((isc_infunk>>8)&0xff);
	val[3]=(byte_t)((isc_infunk>>16)&0xff);
	val[4]=(byte_t)((isc_infunk>>24)&0xff);
	return appendInfoItem(isc_info_error,val,sizeof(val));
}

bool sqlrprotocol_firebird::infoDatabase() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		db handle
	// 	int32_t		object id
	// 	int32_t		requested db info items length
	// 	byte_t[]	requested db info items
	// 	int32_t		response buffer length
	// }

	debugStart("info database");

	uint32_t	bytesread=0;

	// get database handle
	uint32_t	clientdbhandle;
	if (!readInt(&clientdbhandle,"db handle",&bytesread)) {
		return false;
	}

	// get object id
	uint32_t	objectid;
	if (!readInt(&objectid,"object id",&bytesread)) {
		return false;
	}

	// get requested db info items
	uint32_t	dbinfolen;
	byte_t		*dbinfo;
	if (!readBuffer(&dbinfo,&dbinfolen,
			"requested db info items",&bytesread)) {
		return false;
	}

	// get response buffer length
	if (!readInt(&respbufferlen,"response buffer length",&bytesread)) {
		delete[] dbinfo;
		return false;
	}
	fixupRespBufferLen();

	// process requested db info items
	const byte_t	*dbinfoptr=dbinfo;
	const byte_t	*dbinfoendptr=dbinfo+dbinfolen;

	// build the response buffer...
	// (info items are bare bytes in the request, but each value in the
	// response is a cluster - item byte, 2 byte little-endian length, value)
	respbuffer.clear();
	bool	end=false;
	bool	fits=true;
	while (dbinfoptr<dbinfoendptr && !end && fits) {

		// get the requested db info item
		byte_t	dbinfoitem;
		read(dbinfoptr,&dbinfoitem,&dbinfoptr);
		debugDbInfoItem(dbinfoitem);

		switch (dbinfoitem) {
			case isc_info_end:
				// there might be multiple of these, but if we
				// hit one of them then just bail
				end=true;
				break;

			case isc_info_db_id:
				{
				// the database file, then the host it's on,
				// twice - the .NET provider reads 3 entries
				// unconditionally
				const char	*dbid[3]={
						(db)?db:"",
						cont->getDbHostName(),
						cont->getDbHostName()};
				fits=appendInfoStrings(dbinfoitem,dbid,3);
				}
				break;

			case isc_info_implementation:
				{
				// a count, then one (implementation, class)
				// pair per entry.  what a linux firebird 2.5
				// server sends.
				static const byte_t	impl[]=
						{2,0x42,1,0x42,4};
				fits=appendInfoItem(dbinfoitem,
							impl,sizeof(impl));
				}
				break;

			case isc_info_isc_version:
			case isc_info_firebird_version:
				{
				const char	*ver[1]={cont->getDbVersion()};
				fits=appendInfoStrings(dbinfoitem,ver,1);
				}
				break;

			case isc_info_base_level:
				{
				// a count, then the engine's base level and
				// the remote server's own.  the client merges
				// its own in on top of this.
				static const byte_t	baselevel[]={2,6,1};
				fits=appendInfoItem(dbinfoitem,baselevel,
							sizeof(baselevel));
				}
				break;

			case isc_info_page_size:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_PAGE_SIZE);
				break;

			case isc_info_num_buffers:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_NUM_BUFFERS);
				break;

			case isc_info_attachment_id:
				fits=appendInfoInt(dbinfoitem,dbhandle);
				break;

			case isc_info_sweep_interval:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_SWEEP_INTERVAL);
				break;

			case isc_info_ods_version:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_ODS_VERSION);
				break;

			case isc_info_ods_minor_version:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_ODS_MINOR_VERSION);
				break;

			case isc_info_no_reserve:
				fits=appendInfoByte(dbinfoitem,0);
				break;

			case isc_info_forced_writes:
				fits=appendInfoByte(dbinfoitem,1);
				break;

			case isc_info_db_sql_dialect:
				fits=appendInfoByte(dbinfoitem,
						FIREBIRD_SQL_DIALECT);
				break;

			case isc_info_db_read_only:
				fits=appendInfoByte(dbinfoitem,0);
				break;

			case isc_info_db_class:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_DB_CLASS);
				break;

			case isc_info_db_provider:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_DB_PROVIDER);
				break;

			case frb_info_att_charset:
				// the module transliterates nothing, so the
				// character set is NONE
				// (isql takes an isc_info_error for any item
				// but isc_info_firebird_version as proof of a
				// pre-interbase-6 server, and resets its sql
				// dialect to 1)
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_CHARACTER_SET);
				break;

			case isc_info_reads:
			case isc_info_writes:
			case isc_info_fetches:
			case isc_info_marks:
			case isc_info_page_errors:
			case isc_info_record_errors:
			case isc_info_bpage_errors:
			case isc_info_dpage_errors:
			case isc_info_ipage_errors:
			case isc_info_ppage_errors:
			case isc_info_tpage_errors:
				// counters SQL Relay doesn't keep
				fits=appendInfoInt(dbinfoitem,0);
				break;

			case isc_info_read_seq_count:
			case isc_info_read_idx_count:
			case isc_info_insert_count:
			case isc_info_update_count:
			case isc_info_delete_count:
			case isc_info_backout_count:
			case isc_info_purge_count:
			case isc_info_expunge_count:
				// per-relation counters - a real server sends
				// a vector of (2 byte relation id, 4 byte
				// count) pairs in one cluster, and an empty
				// one for relations it hasn't touched
				fits=appendInfoItem(dbinfoitem,NULL,0);
				break;

			case isc_info_allocation:
			case isc_info_db_size_in_pages:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_DB_SIZE_IN_PAGES);
				break;

			case isc_info_current_memory:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_CURRENT_MEMORY);
				break;

			case isc_info_max_memory:
				fits=appendInfoInt(dbinfoitem,
						FIREBIRD_MAX_MEMORY);
				break;

			case isc_info_set_page_buffers:
			case isc_info_db_file_size:
				// what a real server sends for these on a
				// plain read request
				fits=appendInfoInt(dbinfoitem,0);
				break;

			case isc_info_oldest_transaction:
			case isc_info_oldest_active:
			case isc_info_oldest_snapshot:
				// one transaction per session, so one handle
				// answers all three - the same source
				// infoTransaction() answers the isc_info_tra_*
				// items from, so the two can't disagree
				fits=appendInfoInt(dbinfoitem,trhandle);
				break;

			case isc_info_next_transaction:
				// transaction() hands out handles by
				// incrementing this one
				fits=appendInfoInt(dbinfoitem,trhandle+1);
				break;

			case isc_info_active_tran_count:
				fits=appendInfoInt(dbinfoitem,
							(intransaction)?1:0);
				break;

			case isc_info_active_transactions:
				// one cluster per active transaction
				if (intransaction) {
					fits=appendInfoInt(dbinfoitem,
								trhandle);
				}
				break;

			case isc_info_limbo:
				// one cluster per transaction in limbo.  A
				// real server with none sends no cluster at
				// all rather than an empty one.
				break;

			case isc_info_creation_date:
				fits=appendInfoTimestamp(dbinfoitem,
						FIREBIRD_CREATION_YEAR,
						FIREBIRD_CREATION_MONTH,
						FIREBIRD_CREATION_DAY,
						0,0,0);
				break;

			case isc_info_user_names:
				// one cluster per attached user, and the
				// module has one session per attachment, like
				// a classic-mode server
				// (a real server always names someone, so an
				// attach with no user name in its dpb gets the
				// same stand-in appendInfoDescribe() uses)
				fits=appendInfoCountedString(dbinfoitem,
					(charstring::isNullOrEmpty(username))?
							"SYSDBA":username);
				break;

			default:
				// an item a real server wouldn't answer
				// either - isc_info_window_turns,
				// isc_info_license, and the wal and log-file
				// families that predate firebird
				// (nothing else lands here, on purpose - see
				// the isql note on frb_info_att_charset above)
				fits=appendInfoError(dbinfoitem);
				break;
		}
	}

	// a reply that wasn't truncated ends with a bare isc_info_end
	// (the truncation check holds a byte back for it)
	if (fits && respbuffer.getSize()<respbufferlen) {
		write(&respbuffer,(byte_t)isc_info_end);
	}

	debugEnd();

	// clean up
	delete[] dbinfo;

	// status vector...
	successStatusVector();

	return genericResponse("info database response",
				dbhandle,((uint64_t)objectid)<<32,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::disconnect() {

	// no response packet

	debugStart("disconnect");
	debugEnd();

	// return false on purpose here
	return false;
}

bool sqlrprotocol_firebird::transaction() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		db handle
	// 	int32_t		tx parameters buffer length
	// 	byte_t[]	tx parameters buffer
	// }

	debugStart("transaction");

	uint32_t	bytesread=0;

	// get database handle
	uint32_t	clientdbhandle;
	if (!readInt(&clientdbhandle,"db handle",&bytesread)) {
		return false;
	}

	// get tx parameters buffer
	uint32_t	tpblen;
	byte_t		*tpb;
	if (!readBuffer(&tpb,&tpblen,"tx param buffer",&bytesread)) {
		return false;
	}

	// process tx parameters buffer...
	const byte_t	*tpbptr=tpb;
	const byte_t	*tpbendptr=tpb+tpblen;

	// get the tpb version
	if (tpbptr) {
		byte_t		tpbversion;
		read(tpbptr,&tpbversion,&tpbptr);
		debugTpbVersion(tpbversion);
	}

	// what the tpb asked for
	bool	readonly=false;
	bool	autocommit=false;
	byte_t	isolevel=0;

	// get each parameter...
	// (the test is < rather than != because an item whose length walks
	// past the end would otherwise never land on the end pointer, and the
	// loop would run off the buffer - the dpb walk bounds the same case
	// with an explicit over-run check)
	while (tpbptr<tpbendptr) {

		// get the parameter
		byte_t	tpbparam;
		read(tpbptr,&tpbparam,&tpbptr);
		debugTpbParam(tpbparam);

		// process the parameter...
		switch (tpbparam) {
			case isc_tpb_read:
				readonly=true;
				break;

			case isc_tpb_write:
				readonly=false;
				break;

			case isc_tpb_autocommit:
				autocommit=true;
				break;

			case isc_tpb_consistency:
			case isc_tpb_concurrency:
			case isc_tpb_read_committed:
				// last one wins, same as isc_tpb_read/write
				isolevel=tpbparam;
				break;

			case isc_tpb_lock_read:
			case isc_tpb_lock_write:
			case isc_tpb_lock_timeout:
			case isc_tpb_at_snapshot_number:
			case isc_tpb_lock_table_schema:
				{
				// these carry a 1-byte length and that many
				// bytes of value, and have to be skipped by
				// length or the walk desynchronizes
				if (tpbptr>=tpbendptr) {
					break;
				}
				byte_t	tpbvaluelen;
				read(tpbptr,&tpbvaluelen,&tpbptr);
				tpbptr+=tpbvaluelen;
				}
				break;

			default:
				// the rest are bare bytes with no per-
				// transaction handling on this side, and are
				// just read and dropped
				break;
		}
	}

	debugEnd();

	// clean up
	delete[] tpb;

	// every handle names the same underlying transaction - SQL Relay has
	// one per session, but a client is free to ask for several (isql asks
	// for one in sql and then asks again with this op)
	if (!intransaction) {

		// hint the backend before starting the transaction, so a
		// backend that can honor it (eg. firebird's isc_tpb_read)
		// gets the benefits of a read-only transaction, not just the
		// client-visible refusal of writes below
		cont->setReadOnly(readonly);

		// hint the isolation level too - unlike read-only, there's no
		// client-visible substitute for this, so whether it actually
		// takes effect depends entirely on whether the backend honors
		// the hint
		const char	*isolevelname=NULL;
		switch (isolevel) {
			case isc_tpb_consistency:
				isolevelname="TRANSACTION_SERIALIZABLE";
				break;
			case isc_tpb_concurrency:
				isolevelname="TRANSACTION_REPEATABLE_READ";
				break;
			case isc_tpb_read_committed:
				isolevelname="TRANSACTION_READ_COMMITTED";
				break;
		}
		bool	isolevelhonored=cont->setTransactionIsolationLevel(
					isolevelname,
					SQLRSERVERISOLATIONLEVELFORMAT_JDBC);

		// autocommit and an explicit transaction are alternatives, so
		// a tpb that asks for autocommit begins nothing
		bool	started=(autocommit)?
				cont->setAutoCommitOn():
				(cont->setAutoCommitOff() && cont->begin());
		if (!started) {
			return sendCursorError("transaction response",
								NULL,false);
		}

		intransaction=true;
		trautocommit=autocommit;
		trreadonly=readonly;
		trisolevel=(isolevelhonored)?isolevel:0;
	}

	// a client only ever compares a handle against 0, but distinct ones
	// keep a stale handle from looking live in a debug log
	trhandle++;

	if (getDebug()) {
		stdoutput.printf("	transaction handle: %u\n",trhandle);
		stdoutput.printf("	read only: %s\n",(readonly)?"yes":"no");
		stdoutput.printf("	autocommit: %s\n",(autocommit)?"yes":"no");
		stdoutput.printf("	isolation level: %s\n",
			(trisolevel==isc_tpb_consistency)?
				"consistency" :
			(trisolevel==isc_tpb_concurrency)?
				"concurrency" :
			(trisolevel==isc_tpb_read_committed)?
				"read committed" :
				"connection default");
	}

	// status vector...
	successStatusVector();

	return genericResponse("transaction response",
				trhandle,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::commit() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// }

	debugStart("commit");

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	debugEnd();

	// a commit with nothing open is a no-op - the client may hold a handle
	// from a transaction something else already ended
	if (!intransaction) {
		successStatusVector();
		return genericResponse("commit response",
					0,0,
					NULL,0,
					statusvector,statusvectorstr,
					statusvectorlen);
	}

	if (!trautocommit && !cont->commit()) {
		return sendCursorError("commit response",NULL,false);
	}

	// the statements a transaction opened cursors for are done with
	clearStatements();

	// a blob or array id lives until the transaction that made it ends
	clearBlobs();
	clearArrays();

	intransaction=false;
	trautocommit=false;
	trreadonly=false;
	trisolevel=0;

	successStatusVector();

	return genericResponse("commit response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::rollback() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// }

	debugStart("rollback");

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	debugEnd();

	if (!intransaction) {
		successStatusVector();
		return genericResponse("rollback response",
					0,0,
					NULL,0,
					statusvector,statusvectorstr,
					statusvectorlen);
	}

	if (!trautocommit && !cont->rollback()) {
		return sendCursorError("rollback response",NULL,false);
	}

	clearStatements();

	// a blob or array id lives until the transaction that made it ends
	clearBlobs();
	clearArrays();

	intransaction=false;
	trautocommit=false;
	trreadonly=false;
	trisolevel=0;

	successStatusVector();

	return genericResponse("rollback response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::commitRetaining() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// }

	debugStart("commit retaining");

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	debugEnd();

	if (!intransaction) {
		successStatusVector();
		return genericResponse("commit retaining response",
					0,0,
					NULL,0,
					statusvector,statusvectorstr,
					statusvectorlen);
	}

	// commit, then start another - the handle the client holds stays live
	if (!trautocommit && !(cont->commit() && cont->begin())) {
		return sendCursorError("commit retaining response",NULL,false);
	}

	clearStatements();

	successStatusVector();

	return genericResponse("commit retaining response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::rollbackRetaining() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// }

	debugStart("rollback retaining");

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	debugEnd();

	if (!intransaction) {
		successStatusVector();
		return genericResponse("rollback retaining response",
					0,0,
					NULL,0,
					statusvector,statusvectorstr,
					statusvectorlen);
	}

	if (!trautocommit && !(cont->rollback() && cont->begin())) {
		return sendCursorError("rollback retaining response",NULL,false);
	}

	clearStatements();

	successStatusVector();

	return genericResponse("rollback retaining response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

// op_prepare and op_prepare2 are two-phase commit's first phase, a durable
// promise that the second phase can't fail. The server api has no
// distributed-transaction support to back that promise with.
bool sqlrprotocol_firebird::prepare() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::prepare2() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::transactionInfo() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// 	int32_t		incarnation
	// 	int32_t		requested tx info items length
	// 	byte_t[]	requested tx info items
	// 	int32_t		response buffer length
	// }

	debugStart("info transaction");

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	uint32_t	incarnation;
	if (!readInt(&incarnation,"incarnation",&bytesread)) {
		return false;
	}

	uint32_t	trinfolen;
	byte_t		*trinfo;
	if (!readBuffer(&trinfo,&trinfolen,
			"requested tx info items",&bytesread)) {
		return false;
	}

	if (!readInt(&respbufferlen,"response buffer length",&bytesread)) {
		delete[] trinfo;
		return false;
	}
	fixupRespBufferLen();

	// process requested tx info items
	const byte_t	*trinfoptr=trinfo;
	const byte_t	*trinfoendptr=trinfo+trinfolen;

	respbuffer.clear();
	bool	end=false;
	bool	fits=true;
	while (trinfoptr<trinfoendptr && !end && fits) {

		byte_t	trinfoitem;
		read(trinfoptr,&trinfoitem,&trinfoptr);

		switch (trinfoitem) {
			case isc_info_end:
				end=true;
				break;

			case isc_info_tra_id:
			case isc_info_tra_oldest_interesting:
			case isc_info_tra_oldest_snapshot:
			case isc_info_tra_oldest_active:
				fits=appendInfoInt(trinfoitem,trhandle);
				break;

			case isc_info_tra_isolation:
				{
				// map the tpb-space isolation byte the
				// transaction was actually started with (or
				// 0, meaning the connection's default, which
				// is read committed) to the separate
				// isc_info_tra_isolation value space
				byte_t	infolevel=isc_info_tra_read_committed;
				if (trisolevel==isc_tpb_consistency) {
					infolevel=isc_info_tra_consistency;
				} else if (trisolevel==isc_tpb_concurrency) {
					infolevel=isc_info_tra_concurrency;
				}
				if (infolevel==isc_info_tra_read_committed) {
					// read committed carries a second byte
					// naming the record-version sub-option
					byte_t	isolevelbuf[2]={
						infolevel,
						isc_info_tra_rec_version
						};
					fits=appendInfoItem(trinfoitem,
							isolevelbuf,2);
				} else {
					fits=appendInfoByte(trinfoitem,
							infolevel);
				}
				}
				break;

			case isc_info_tra_access:
				fits=appendInfoByte(trinfoitem,
						(trreadonly)?
						isc_info_tra_readonly:
						isc_info_tra_readwrite);
				break;

			case isc_info_tra_lock_timeout:
				fits=appendInfoInt(trinfoitem,0);
				break;

			default:
				fits=appendInfoError(trinfoitem);
				break;
		}
	}

	if (fits && respbuffer.getSize()<respbufferlen) {
		write(&respbuffer,(byte_t)isc_info_end);
	}

	debugEnd();

	// clean up
	delete[] trinfo;

	successStatusVector();

	return genericResponse("info transaction response",
				clienttrhandle,0,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::allocateStatement() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		db handle
	// }

	debugStart("allocate statement");

	uint32_t	bytesread=0;

	uint32_t	clientdbhandle;
	if (!readInt(&clientdbhandle,"db handle",&bytesread)) {
		return false;
	}

	// get an available cursor
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		debugEnd();
		return errorResponse("allocate statement response",
					isc_dsql_error,"HY000",-901,
					"Out of cursors",14);
	}

	// the wire format only ever binds by "?", so a query has to be
	// translated to whatever format the backend actually requires
	cont->setRequireBindVariableTranslation(cursor,true);

	uint16_t	cursorid=cont->getId(cursor);

	// the handle is the cursor id plus one, so it is never 0 - the client
	// treats a 0 handle as unallocated
	uint32_t	stmthandle=cursorid+1;

	sqlrfirebirdstatement	*stmt=&statements[cursorid];
	stmt->stmttype=0;
	stmt->prepared=false;
	stmt->preexecuted=false;
	stmt->cursoropen=false;
	delete[] stmt->cursorname;
	stmt->cursorname=NULL;
	delete[] stmt->outfields;
	stmt->outfields=NULL;
	stmt->outfieldcount=0;
	delete[] stmt->binds;
	stmt->binds=NULL;
	stmt->bindcount=0;
	stmt->bindsdescribed=false;
	clearProbeColumns(stmt);
	clearBatch(&stmt->batch);
	clearBlrRequest(&stmt->request);

	if (getDebug()) {
		stdoutput.printf("	statement handle: %u\n",stmthandle);
	}

	debugEnd();

	successStatusVector();

	return genericResponse("allocate statement response",
				stmthandle,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::freeStatement() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	int32_t		option flags
	// }

	debugStart("free statement");

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	option;
	if (!readInt(&option,"option",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt) {
		return errorResponse("free statement response",
					isc_bad_stmt_handle);
	}

	// DSQL_close on a statement with no open cursor is an error rather
	// than a no-op, which is what a real firebird answers
	if ((option&DSQL_close) && !(option&DSQL_drop) && !stmt->cursoropen) {
		return errorResponse("free statement response",
					isc_dsql_cursor_close_err);
	}

	if (option&DSQL_close) {
		cont->closeResultSet(cursor);
		stmt->cursoropen=false;
		stmt->preexecuted=false;
	}

	if (option&DSQL_unprepare) {
		cont->closeResultSet(cursor);
		stmt->cursoropen=false;
		stmt->preexecuted=false;
		stmt->prepared=false;
	}

	uint32_t	objecthandle=stmthandle;
	if (option&DSQL_drop) {
		clearStatement(cont->getId(cursor));
		cont->abort(cursor);
		cont->release(cursor);
		// what a real firebird answers for a dropped statement
		objecthandle=0xffff;
	}

	successStatusVector();

	return genericResponse("free statement response",
				objecthandle,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::setCursor() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	char[]		cursor name
	// 	int32_t		type
	// }

	debugStart("set cursor");

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	char	*cursorname=NULL;
	if (!readString(&cursorname,"cursor name",&bytesread)) {
		return false;
	}

	uint32_t	type;
	if (!readInt(&type,"type",&bytesread)) {
		delete[] cursorname;
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt) {
		delete[] cursorname;
		return errorResponse("set cursor response",
					isc_bad_stmt_handle);
	}

	// the name is kept but never handed to the backend - SQL Relay has no
	// way to name a backend cursor
	// (firebird only takes a name between prepare and execute, and against
	// a backend that can't describe a prepared statement it would be too
	// late here anyway, since runPreparedQuery() has already run a select
	// with no binds and opened the backend's cursor.  naming and fetching
	// works; "where current of" fails at the backend with the -504 a real
	// server sends for a cursor that doesn't exist.)
	delete[] stmt->cursorname;
	stmt->cursorname=cursorname;

	successStatusVector();

	return genericResponse("set cursor response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::prepareStatement() {
	return prepareOrExecImmediate(false);
}

bool sqlrprotocol_firebird::execImmediate() {
	return prepareOrExecImmediate(true);
}

bool sqlrprotocol_firebird::prepareOrExecImmediate(bool execimmediate) {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// 	int32_t		statement handle
	// 	int32_t		sql dialect
	// 	char[]		sql text
	// 	byte_t[]	requested sql info items
	// 	int32_t		response buffer length
	// }

	debugStart((execimmediate)?"exec immediate":"prepare statement");

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	dialect;
	if (!readInt(&dialect,"dialect",&bytesread)) {
		return false;
	}

	char		*query=NULL;
	uint32_t	querylen=0;
	if (!readString(&query,&querylen,"query",&bytesread)) {
		return false;
	}

	uint32_t	itemslen=0;
	byte_t		*items=NULL;
	if (!readBuffer(&items,&itemslen,
			"requested sql info items",&bytesread)) {
		delete[] query;
		return false;
	}

	if (!readInt(&respbufferlen,"response buffer length",&bytesread)) {
		delete[] query;
		delete[] items;
		return false;
	}
	fixupRespBufferLen();

	debugEnd();

	bool	retval=runPreparedQuery(execimmediate,stmthandle,
					query,querylen,items,itemslen);

	delete[] query;
	delete[] items;

	return retval;
}

bool sqlrprotocol_firebird::runPreparedQuery(bool execimmediate,
						uint32_t stmthandle,
						const char *query,
						uint32_t querylen,
						const byte_t *items,
						uint32_t itemslen) {

	const char	*title=(execimmediate)?
				"exec immediate response":
				"prepare statement response";

	// bounds check, before the query can size anything
	if (querylen>maxquerysize) {
		return errorResponse(title,isc_dsql_error,"54001",-901,
					"Query is too large",18);
	}

	uint32_t	stmttype=statementType(query);

	// a transaction asked for in sql needs no cursor and never reaches
	// the backend
	if (execimmediate && isTransactionStatement(stmttype)) {

		if (!runTransactionStatement(stmttype)) {
			return sendCursorError(title,NULL,false);
		}

		successStatusVector();

		return genericResponse(title,
					trhandle,0,
					NULL,0,
					statusvector,statusvectorstr,
					statusvectorlen);
	}

	// get the cursor
	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=NULL;
	if (execimmediate) {
		// op_exec_immediate carries no statement handle, so it runs on
		// a cursor of its own that goes back to the pool below
		cursor=cont->getCursor();
		if (!cursor) {
			return errorResponse(title,isc_dsql_error,"HY000",-901,
						"Out of cursors",14);
		}
		// the wire format only ever binds by "?", so a query has to
		// be translated to whatever format the backend requires
		cont->setRequireBindVariableTranslation(cursor,true);
	} else {
		stmt=getStatement(stmthandle,&cursor);
		if (!stmt) {
			return errorResponse(title,isc_bad_stmt_handle);
		}
		cont->closeResultSet(cursor);
		stmt->cursoropen=false;
		stmt->preexecuted=false;
		stmt->prepared=false;
		delete[] stmt->outfields;
		stmt->outfields=NULL;
		stmt->outfieldcount=0;
		delete[] stmt->binds;
		stmt->binds=NULL;
		stmt->bindcount=0;
		stmt->bindsdescribed=false;
		clearProbeColumns(stmt);
	}

	// copy the query into the cursor's own buffer
	char	*querybuffer=cont->getQueryBuffer(cursor);
	bytestring::copy(querybuffer,query,querylen);
	querybuffer[querylen]='\0';
	cont->setQuerySize(cursor,querylen);

	// op_exec_immediate2 filled the binds before the query arrived, so
	// only clear them when nothing did
	if (!cont->getInputBindCount(cursor)) {
		cont->getBindPool(cursor)->clear();
		cont->setInputBindCount(cursor,0);
	}

	// prepare
	// (a transaction asked for in sql is driven through the controller by
	// execute(), so there is nothing here for the backend to prepare)
	if (!isTransactionStatement(stmttype) &&
		!cont->prepareQuery(cursor,querybuffer,querylen,
					true,true,true,true)) {
		bool	retval=sendCursorError(title,cursor,true);
		if (execimmediate) {
			cont->release(cursor);
		}
		return retval;
	}

	// refuse the write here - SQL Relay can't ask a backend for a
	// read-only transaction
	// (after the prepare, and only for exec immediate, because that is
	// where a real server refuses.  a prepare in a read-only transaction
	// succeeds, and one against a missing table still has to fail with
	// the backend's own error.  a real server refuses at the moment a
	// record is modified, so an update matching no rows succeeds there
	// and is refused here.  it sends nothing but the bare
	// isc_read_only_trans, and the client turns that one code into sqlcode
	// -817, sqlstate 42000 and "attempted update during read-only
	// transaction" out of its own tables.  recognized ddl is refused here
	// too, but with the compound reply a real server sends for it - see
	// readOnlyMetaUpdateResponse().)
	if (execimmediate && trreadonly) {
		uint32_t	dyncode=0;
		if (ddlDynCode(query,&dyncode)) {
			cont->release(cursor);
			return readOnlyMetaUpdateResponse(title,dyncode,
							ddlobjectname);
		}
		if (isWriteStatement(stmttype)) {
			cont->release(cursor);
			return errorResponse(title,isc_read_only_trans);
		}
	}

	// a firebird client expects op_prepare_statement to answer with the
	// shape of the result set, so when the columns aren't already known a
	// select with nothing to bind is run here and execute() knows not to
	// run it a second time
	// (a select with binds can't be, and describes as no columns.  the
	// test is colCount() rather than columnInfoIsValidAfterPrepare()
	// because that one answers what the backend's cursor class can do,
	// not what this prepare did - prepareQuery() has three paths that
	// skip the describe on a backend whose class says it describes, and
	// colCount() answers 0 in exactly those cases)
	bool	executed=false;
	if (execimmediate ||
		((stmttype==isc_info_sql_stmt_select ||
			stmttype==isc_info_sql_stmt_select_for_upd) &&
			!cont->colCount(cursor) &&
			!cont->countBindVariables(querybuffer,querylen))) {

		if (!cont->executeQuery(cursor,true,true,true,true)) {
			bool	retval=sendCursorError(title,cursor,false);
			if (execimmediate) {
				cont->release(cursor);
			}
			return retval;
		}
		executed=true;
	}

	// a select with binds can't be pre-executed above, and with faked
	// binds there is no prepared statement at the backend to describe
	// either, so the shape of the result set is worked out here instead,
	// on a cursor of its own, from a copy of the query with NULL in place
	// of every bind
	// (the client's own cursor is left alone, and executes the real query
	// with the real binds later.  the intercept path prepareQuery() also
	// returns early from needs no test of its own - it only ever takes
	// begin/commit/rollback/autocommit/set, never a select)
	if (!execimmediate && !executed && stmt &&
		(stmttype==isc_info_sql_stmt_select ||
			stmttype==isc_info_sql_stmt_select_for_upd) &&
		!cont->colCount(cursor) &&
		cont->countBindVariables(querybuffer,querylen) &&
		cont->getFakeInputBindsForThisQuery(cursor)) {

		describeOutputColumns(cursor,stmt);
	}

	if (stmt) {
		stmt->stmttype=stmttype;
		stmt->prepared=true;
		stmt->preexecuted=executed;
		stmt->cursoropen=executed && cont->colCount(cursor)>0;
	}

	// build the reply the requested info items ask for
	respbuffer.clear();
	if (!execimmediate && itemslen) {
		if (stmt) {
			describeBinds(cursor,stmt,items,itemslen);
		}
		buildSqlInfo(cursor,stmt,items,itemslen);
	}

	if (execimmediate) {
		cont->release(cursor);
	}

	successStatusVector();

	// op_exec_immediate answers with the transaction handle, and
	// op_prepare_statement with a statement flag mask
	uint32_t	objecthandle=trhandle;
	if (!execimmediate) {
		objecthandle=FB_STMT_REPEAT_EXECUTE;
		if (stmttype==isc_info_sql_stmt_select ||
			stmttype==isc_info_sql_stmt_select_for_upd) {
			objecthandle|=FB_STMT_HAS_CURSOR;
		}
	}

	return genericResponse(title,
				objecthandle,0,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::execute() {
	return executeStatement(false);
}

bool sqlrprotocol_firebird::execute2() {
	return executeStatement(true);
}

bool sqlrprotocol_firebird::executeStatement(bool isexecute2) {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	int32_t		transaction handle
	// 	byte_t[]	input message blr
	// 	int32_t		input message number
	// 	int32_t		input message count
	// 	byte_t[]	input message	(only if the count is non-zero)
	// 	byte_t[]	output message blr	(op_execute2 only)
	// 	int32_t		output message number	(op_execute2 only)
	// 	int32_t		statement timeout	(protocol 16 and up)
	// }

	debugStart((isexecute2)?"execute2":"execute");

	const char	*title=(isexecute2)?"execute2 response":
						"execute response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	sqlrfirebirdfield	*infields=NULL;
	uint16_t		infieldcount=0;
	uint32_t		blrgdscode=0;
	if (!readBlr(&infields,&infieldcount,"input blr",
					&bytesread,&blrgdscode)) {
		// the rest of the request is still on the socket, so the
		// session ends either way - but the client hears why first
		if (blrgdscode) {
			errorResponse(title,blrgdscode);
		}
		return false;
	}

	uint32_t	inmsgnumber;
	if (!readInt(&inmsgnumber,"input message number",&bytesread)) {
		delete[] infields;
		return false;
	}

	uint32_t	inmsgcount;
	if (!readInt(&inmsgcount,"input message count",&bytesread)) {
		delete[] infields;
		return false;
	}

	// find the statement before the message, so a bad handle doesn't
	// leave the message on the socket
	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);

	// readMessage() sets this, and only runs when there is a message
	badblobid=false;

	bool	messageread=true;
	if (inmsgcount && infieldcount) {
		// the binds are about to be refilled from the message, so
		// whatever the last execute allocated can go - without this a
		// statement executed in a loop grows the pool every time
		// (clearing inside the "if" is deliberate - nothing refills
		// the binds when there is no message, and the backend is
		// still pointing at them)
		if (cursor) {
			cont->getBindPool(cursor)->clear();
		}
		messageread=readMessage(cursor,infields,infieldcount,
								&bytesread);
	}
	delete[] infields;
	if (!messageread) {
		return false;
	}

	sqlrfirebirdfield	*outfields=NULL;
	uint16_t		outfieldcount=0;
	if (isexecute2) {
		if (!readBlr(&outfields,&outfieldcount,
					"output blr",&bytesread,&blrgdscode)) {
			if (blrgdscode) {
				errorResponse(title,blrgdscode);
			}
			return false;
		}
		uint32_t	outmsgnumber;
		if (!readInt(&outmsgnumber,
				"output message number",&bytesread)) {
			delete[] outfields;
			return false;
		}
	}

	// get statement timeout
	// (protocol 16 appended it to op_execute/op_execute2 - see firebird's
	// src/remote/protocol.cpp, PROTOCOL_STMT_TOUT in the op_execute case.
	// it has to be read even though nothing here honors it, or the rest of
	// the packet would be misread as the next request)
	if (protocolversion>=PROTOCOL_VERSION16) {
		uint32_t	stmttimeout;
		if (!readInt(&stmttimeout,"statement timeout",&bytesread)) {
			delete[] outfields;
			return false;
		}
	}

	debugEnd();

	if (!stmt || !stmt->prepared) {
		delete[] outfields;
		return errorResponse(title,isc_bad_stmt_handle);
	}

	if (badblobid) {
		delete[] outfields;
		return errorResponse(title,isc_bad_segstr_id);
	}

	// refuse a write in a read-only transaction - see runPreparedQuery()
	// (the statement's text is still in the cursor's query buffer, where
	// the prepare put it, which is what the ddl test needs)
	if (trreadonly) {
		uint32_t	dyncode=0;
		if (cursor && ddlDynCode(cont->getQueryBuffer(cursor),
							&dyncode)) {
			delete[] outfields;
			return readOnlyMetaUpdateResponse(title,dyncode,
							ddlobjectname);
		}
		if (isWriteStatement(stmt->stmttype)) {
			delete[] outfields;
			return errorResponse(title,isc_read_only_trans);
		}
	}

	// run it, unless prepareStatement() already did
	if (stmt->preexecuted) {
		stmt->preexecuted=false;
	} else if (isTransactionStatement(stmt->stmttype)) {
		if (!runTransactionStatement(stmt->stmttype)) {
			delete[] outfields;
			return sendCursorError(title,NULL,false);
		}
	} else if (!cont->executeQuery(cursor,true,true,true,true)) {
		delete[] outfields;
		return sendCursorError(title,cursor,false);
	}

	stmt->cursoropen=(cont->colCount(cursor)>0);

	// op_execute2 answers the singleton output row ahead of the response
	if (isexecute2) {
		bool	sent=sendSqlResponse(cursor,outfields,outfieldcount);
		delete[] outfields;
		if (!sent) {
			return false;
		}
	}

	successStatusVector();

	return genericResponse(title,
				trhandle,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::sendSqlResponse(sqlrservercursor *cursor,
						sqlrfirebirdfield *fields,
						uint16_t fieldcount) {

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_sql_response
	// 	int32_t		message count
	// 	byte_t[]	message		(only if the count is non-zero)
	// }

	debugStart("sql response");

	uint32_t	byteswritten=0;

	bool	error=false;
	bool	haverow=fieldcount && cont->colCount(cursor) &&
					cont->fetchRow(cursor,&error);

	opcode=op_sql_response;
	if (!writeInt(opcode,"response op code",&byteswritten)) {
		return false;
	}
	debugOpCode("response op code",opcode);

	if (!writeInt((haverow)?1:0,"message count",&byteswritten)) {
		return false;
	}

	if (haverow) {
		if (!writeMessage(cursor,fields,fieldcount,&byteswritten)) {
			return false;
		}
		cont->nextRow(cursor);
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

bool sqlrprotocol_firebird::execImmediate2() {

	// request packet data structure:
	//
	// data {
	// 	byte_t[]	input message blr
	// 	int32_t		input message number
	// 	int32_t		input message count
	// 	byte_t[]	input message	(only if the count is non-zero)
	// 	byte_t[]	output message blr
	// 	int32_t		output message number
	// 	int32_t		transaction handle
	// 	int32_t		statement handle
	// 	int32_t		sql dialect
	// 	char[]		sql text
	// 	byte_t[]	requested sql info items
	// 	int32_t		response buffer length
	// }
	//
	// (the message fields come first and the rest is op_exec_immediate's
	// request, which is how firebird's own decoder reads it - the case
	// falls through)

	debugStart("exec immediate2");

	uint32_t	bytesread=0;

	sqlrfirebirdfield	*infields=NULL;
	uint16_t		infieldcount=0;
	uint32_t		blrgdscode=0;
	if (!readBlr(&infields,&infieldcount,"input blr",
					&bytesread,&blrgdscode)) {
		if (blrgdscode) {
			errorResponse("exec immediate2 response",blrgdscode);
		}
		return false;
	}

	uint32_t	inmsgnumber;
	if (!readInt(&inmsgnumber,"input message number",&bytesread)) {
		delete[] infields;
		return false;
	}

	uint32_t	inmsgcount;
	if (!readInt(&inmsgcount,"input message count",&bytesread)) {
		delete[] infields;
		return false;
	}

	// the message arrives ahead of the query, so it has to be bound to a
	// cursor before there is anything to run on it
	// (binds live on the cursor rather than the query, so filling first
	// and preparing after works - see runPreparedQuery())
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		delete[] infields;
		return errorResponse("exec immediate2 response",
					isc_dsql_error,"HY000",-901,
					"Out of cursors",14);
	}
	// the wire format only ever binds by "?", so a query has to be
	// translated to whatever format the backend actually requires
	cont->setRequireBindVariableTranslation(cursor,true);
	cont->getBindPool(cursor)->clear();
	cont->setInputBindCount(cursor,0);

	// readMessage() sets this, and only runs when there is a message
	badblobid=false;

	bool	messageread=true;
	if (inmsgcount && infieldcount) {
		messageread=readMessage(cursor,infields,infieldcount,
								&bytesread);
	}
	delete[] infields;
	if (!messageread) {
		cont->release(cursor);
		return false;
	}

	sqlrfirebirdfield	*outfields=NULL;
	uint16_t		outfieldcount=0;
	if (!readBlr(&outfields,&outfieldcount,"output blr",
					&bytesread,&blrgdscode)) {
		cont->release(cursor);
		if (blrgdscode) {
			errorResponse("exec immediate2 response",blrgdscode);
		}
		return false;
	}

	uint32_t	outmsgnumber;
	if (!readInt(&outmsgnumber,"output message number",&bytesread)) {
		delete[] outfields;
		cont->release(cursor);
		return false;
	}

	// this op shares a struct with op_prepare_statement, so it carries
	// requested info items and a response buffer length that govern
	// nothing here - its reply is an sql response, not an info response
	// (they still have to come off the socket, but the length goes into a
	// local rather than the member, which only an info response reads)
	uint32_t	clienttrhandle;
	uint32_t	stmthandle;
	uint32_t	dialect;
	char		*query=NULL;
	uint32_t	querylen=0;
	uint32_t	itemslen=0;
	byte_t		*items=NULL;
	uint32_t	unusedrespbufferlen=0;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread) ||
		!readInt(&stmthandle,"statement handle",&bytesread) ||
		!readInt(&dialect,"dialect",&bytesread) ||
		!readString(&query,&querylen,"query",&bytesread) ||
		!readBuffer(&items,&itemslen,
				"requested sql info items",&bytesread) ||
		!readInt(&unusedrespbufferlen,
				"response buffer length",&bytesread)) {
		delete[] query;
		delete[] items;
		delete[] outfields;
		cont->release(cursor);
		return false;
	}

	debugEnd();

	bool	retval=(badblobid)?
			errorResponse("exec immediate2 response",
						isc_bad_segstr_id):
			runOnCursor(cursor,"exec immediate2 response",
					query,querylen,
					outfields,outfieldcount);

	delete[] query;
	delete[] items;
	delete[] outfields;
	cont->release(cursor);

	return retval;
}

bool sqlrprotocol_firebird::runOnCursor(sqlrservercursor *cursor,
					const char *title,
					const char *query,
					uint32_t querylen,
					sqlrfirebirdfield *outfields,
					uint16_t outfieldcount) {

	if (querylen>maxquerysize) {
		return errorResponse(title,isc_dsql_error,"54001",-901,
					"Query is too large",18);
	}

	char	*querybuffer=cont->getQueryBuffer(cursor);
	bytestring::copy(querybuffer,query,querylen);
	querybuffer[querylen]='\0';
	cont->setQuerySize(cursor,querylen);

	if (!cont->prepareQuery(cursor,querybuffer,querylen,
					true,true,true,true)) {
		return sendCursorError(title,cursor,true);
	}

	// refuse a write in a read-only transaction - see runPreparedQuery()
	if (trreadonly) {
		uint32_t	dyncode=0;
		if (ddlDynCode(query,&dyncode)) {
			return readOnlyMetaUpdateResponse(title,dyncode,
							ddlobjectname);
		}
		if (isWriteStatement(statementType(query))) {
			return errorResponse(title,isc_read_only_trans);
		}
	}

	if (!cont->executeQuery(cursor,true,true,true,true)) {
		return sendCursorError(title,cursor,false);
	}

	if (outfieldcount &&
		!sendSqlResponse(cursor,outfields,outfieldcount)) {
		return false;
	}

	successStatusVector();

	return genericResponse(title,
				trhandle,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::fetch() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	byte_t[]	output message blr	(first fetch only)
	// 	int32_t		message number
	// 	int32_t		how many messages to send
	// }

	debugStart("fetch");

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	sqlrfirebirdfield	*fields=NULL;
	uint16_t		fieldcount=0;
	uint32_t		blrgdscode=0;
	if (!readBlr(&fields,&fieldcount,"output blr",
					&bytesread,&blrgdscode)) {
		if (blrgdscode) {
			errorResponse("fetch response",blrgdscode);
		}
		return false;
	}

	uint32_t	msgnumber;
	if (!readInt(&msgnumber,"message number",&bytesread)) {
		delete[] fields;
		return false;
	}

	uint32_t	msgcount;
	if (!readInt(&msgcount,"message count",&bytesread)) {
		delete[] fields;
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->prepared) {
		delete[] fields;
		return errorResponse("fetch response",isc_bad_stmt_handle);
	}

	// the blr only comes with the first fetch of a cursor, so the format
	// it describes has to outlive the packet that carried it
	if (fieldcount) {
		delete[] stmt->outfields;
		stmt->outfields=fields;
		stmt->outfieldcount=fieldcount;
	} else {
		delete[] fields;
	}

	if (!stmt->outfieldcount) {
		return errorResponse("fetch response",isc_dsql_cursor_err);
	}

	if (!stmt->cursoropen) {
		return errorResponse("fetch response",isc_dsql_cursor_err);
	}

	return sendFetchResponse(cursor,stmt,msgcount);
}

bool sqlrprotocol_firebird::sendFetchResponse(sqlrservercursor *cursor,
						sqlrfirebirdstatement *stmt,
						uint32_t msgcount) {

	// response packet data structure, repeated:
	//
	// data {
	// 	int32_t		op_fetch_response
	// 	int32_t		status	(0 for a row, 100 at the end)
	// 	int32_t		message count
	// 	byte_t[]	message		(only if the count is non-zero)
	// }

	debugStart("fetch response");

	uint32_t	byteswritten=0;
	uint32_t	sent=0;

	for (;;) {

		bool	error=false;
		if (!cont->fetchRow(cursor,&error)) {
			if (error) {
				debugEnd();
				return sendCursorError("fetch response",cursor,false);
			}
			break;
		}

		opcode=op_fetch_response;
		if (!writeInt(opcode,"response op code",&byteswritten) ||
			!writeInt(0,"status",&byteswritten) ||
			!writeInt(1,"message count",&byteswritten) ||
			!writeMessage(cursor,stmt->outfields,
					stmt->outfieldcount,&byteswritten)) {
			return false;
		}

		// FIXME: kludgy
		cont->nextRow(cursor);

		sent++;
		if (msgcount && sent==msgcount) {
			break;
		}
	}

	// a batch that stopped because it filled up ends with status 0 and no
	// message, and one that ran out of rows ends with 100
	opcode=op_fetch_response;
	if (!writeInt(opcode,"response op code",&byteswritten) ||
		!writeInt((msgcount && sent==msgcount)?0:100,
						"status",&byteswritten) ||
		!writeInt(0,"message count",&byteswritten)) {
		return false;
	}

	if (getDebug()) {
		stdoutput.printf("	rows sent: %u\n",sent);
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

bool sqlrprotocol_firebird::compile() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		db handle
	// 	byte_t[]	request blr
	// }

	debugStart("compile");

	uint32_t	bytesread=0;

	uint32_t	clientdbhandle;
	if (!readInt(&clientdbhandle,"db handle",&bytesread)) {
		return false;
	}

	uint32_t	blrlen=0;
	byte_t		*blr=NULL;
	if (!readBuffer(&blr,&blrlen,"request blr",&bytesread)) {
		return false;
	}

	debugEnd();

	// a compiled request runs on a cursor of its own, the same way a
	// prepared statement does
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		delete[] blr;
		return errorResponse("compile response",
					isc_dsql_error,"HY000",-901,
					"Out of cursors",14);
	}

	// the wire format only ever binds by "?", so a query has to be
	// translated to whatever format the backend actually requires
	cont->setRequireBindVariableTranslation(cursor,true);

	uint16_t	cursorid=cont->getId(cursor);

	clearStatement(cursorid);

	sqlrfirebirdstatement	*stmt=&statements[cursorid];

	uint32_t	gdscode=0;
	bool		parsed=parseBlrRequest(blr,blrlen,
						&stmt->request,&gdscode);

	delete[] blr;

	// the whole request has already come off the socket, so a blr the
	// walker couldn't make sense of is just an error to answer, not a
	// reason to end the session
	if (!parsed) {
		clearBlrRequest(&stmt->request);
		cont->release(cursor);
		return errorResponse("compile response",
					(gdscode)?gdscode:isc_wish_list);
	}

	// the handle is the cursor id plus one, so it is never 0 - the client
	// treats a 0 handle as unallocated
	uint32_t	reqhandle=cursorid+1;

	if (getDebug()) {
		stdoutput.printf("	request handle: %u\n",reqhandle);
		stdoutput.printf("	query: %s\n",stmt->request.query);
	}

	successStatusVector();

	return genericResponse("compile response",
				reqhandle,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::startAndReceive() {
	return startRequest(false);
}

bool sqlrprotocol_firebird::startSendAndReceive() {
	return startRequest(true);
}

bool sqlrprotocol_firebird::startRequest(bool send) {

	// request packet data structure:
	//
	// data {
	// 	int32_t		request handle
	// 	int32_t		incarnation
	// 	int32_t		transaction handle
	// 	int32_t		message number
	// 	int32_t		how many messages the client will take
	// 	byte_t[]	input message	(op_start_send_and_receive only)
	// }

	const char	*title=(send)?"start send and receive response":
					"start and receive response";

	debugStart((send)?"start send and receive":"start and receive");

	uint32_t	bytesread=0;

	uint32_t	reqhandle;
	if (!readInt(&reqhandle,"request handle",&bytesread)) {
		return false;
	}

	uint32_t	incarnation;
	if (!readInt(&incarnation,"incarnation",&bytesread)) {
		return false;
	}

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	uint32_t	msgnumber;
	if (!readInt(&msgnumber,"message number",&bytesread)) {
		return false;
	}

	uint32_t	msgcount;
	if (!readInt(&msgcount,"message count",&bytesread)) {
		return false;
	}

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(reqhandle,&cursor);
	sqlrfirebirdblrrequest	*req=(stmt)?&stmt->request:NULL;

	// the input message is laid out the way the request said it would
	// be, so without the request there is no way to take it off the
	// socket, and no way to go on to the next packet either
	if (send && (!req || !req->compiled || !req->hasinmsg ||
					msgnumber!=req->inmsg)) {
		debugEnd();
		errorResponse(title,isc_bad_req_handle);
		return false;
	}

	uint16_t		valuecount=0;
	sqlrfirebirdvalue	*values=NULL;
	if (send) {
		valuecount=req->msgfieldcount[req->inmsg];
		values=new sqlrfirebirdvalue[valuecount];
		for (uint16_t i=0; i<valuecount; i++) {
			values[i].strval=NULL;
		}
		if (!readRequestMessage(req->msgfields[req->inmsg],
					valuecount,values,&bytesread)) {
			for (uint16_t i=0; i<valuecount; i++) {
				delete[] values[i].strval;
			}
			delete[] values;
			return false;
		}
	}

	debugEnd();

	if (!req || !req->compiled) {
		return errorResponse(title,isc_bad_req_handle);
	}

	// a start on a request that already ran rewinds it
	cont->closeResultSet(cursor);
	stmt->cursoropen=false;
	stmt->preexecuted=false;

	// bind whatever the input message carried, in the order the ?s the
	// translation left in the query expect it
	cont->getBindPool(cursor)->clear();
	cont->setInputBindCount(cursor,0);
	if (send) {
		memorypool		*bindpool=cont->getBindPool(cursor);
		sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);
		uint16_t		bindcount=0;
		for (uint16_t i=0; i<req->inparamcount &&
					bindcount<maxbindcount; i++) {
			uint16_t	index=req->inparams[i];
			if (index>=valuecount) {
				continue;
			}
			bindMessageValue(bindpool,&(inbinds[bindcount]),
					bindcount,
					&(req->msgfields[req->inmsg][index]),
					&(values[index]));
			bindcount++;
		}
		cont->setInputBindCount(cursor,bindcount);
		for (uint16_t i=0; i<valuecount; i++) {
			delete[] values[i].strval;
		}
		delete[] values;
		if (getDebug()) {
			stdoutput.printf("	bound %u parameter(s)\n",
								bindcount);
		}
	}

	// run the query op_compile translated the request to
	char	*querybuffer=cont->getQueryBuffer(cursor);
	bytestring::copy(querybuffer,req->query,req->querylen);
	querybuffer[req->querylen]='\0';
	cont->setQuerySize(cursor,req->querylen);

	if (!cont->prepareQuery(cursor,querybuffer,req->querylen,
					true,true,true,true)) {
		return sendCursorError(title,cursor,true);
	}

	if (!cont->executeQuery(cursor,true,true,true,true)) {
		return sendCursorError(title,cursor,false);
	}

	stmt->prepared=true;
	stmt->cursoropen=(cont->colCount(cursor)>0);

	if (!sendStartResponse(req->outmsg)) {
		return false;
	}

	return sendRequestRows(reqhandle,incarnation,cursor,req);
}

bool sqlrprotocol_firebird::sendStartResponse(uint32_t msgnumber) {

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_response_piggyback
	// 	int32_t		the message number the rows come back in
	// 	int32_t		object id
	// 	int32_t		blob id, low word
	// 	int32_t		buffer length
	// 	int32_t[]	status vector
	// }
	//
	// It is the same reply op_response carries, sent under an op code
	// that tells the client the rows are right behind it rather than
	// waiting for an op_receive to ask for them.

	debugStart("start response");

	uint32_t	byteswritten=0;

	opcode=op_response_piggyback;
	if (!writeInt(opcode,"response op code",&byteswritten)) {
		return false;
	}
	debugOpCode("response op code",opcode);

	successStatusVector();

	if (!writeInt(msgnumber,"message number",&byteswritten) ||
		!writeInt(0,"object id",&byteswritten) ||
		!writeInt(0,"blob id low word",&byteswritten) ||
		!writeBuffer(NULL,0,"buffer",&byteswritten) ||
		!writeStatusVector(statusvector,statusvectorstr,
					statusvectorlen,&byteswritten)) {
		return false;
	}

	debugEnd();

	return true;
}

bool sqlrprotocol_firebird::sendRequestRows(uint32_t reqhandle,
					uint32_t incarnation,
					sqlrservercursor *cursor,
					sqlrfirebirdblrrequest *req) {

	// The whole result set goes back in one batch.  A real server sends
	// what fits in a batch and waits for an op_receive to ask for the
	// rest, but every request isql compiles ends with a send that runs
	// after the loop, and the client stops reading at the message that
	// one fills, so it never asks for more.

	debugStart("start rows");

	uint32_t	sent=0;

	for (;;) {

		bool	error=false;
		if (!cont->fetchRow(cursor,&error)) {
			if (error) {
				debugEnd();
				return sendCursorError("start rows",
							cursor,false);
			}
			break;
		}

		if (!sendRequestMessage(reqhandle,incarnation,1,
					cursor,req,req->rowslots)) {
			return false;
		}

		// FIXME: kludgy
		cont->nextRow(cursor);

		sent++;
	}

	// what the request assigns after its loop ends is what the client
	// reads as the end of the stream, and the 0 count says the same
	// thing at the packet level
	if (!sendRequestMessage(reqhandle,incarnation,0,
					cursor,req,req->eofslots)) {
		return false;
	}

	if (getDebug()) {
		stdoutput.printf("	rows sent: %u\n",sent);
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

bool sqlrprotocol_firebird::sendRequestMessage(uint32_t reqhandle,
					uint32_t incarnation,
					uint32_t msgcount,
					sqlrservercursor *cursor,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrslot *slots) {

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_send
	// 	int32_t		request handle
	// 	int32_t		incarnation
	// 	int32_t		transaction handle
	// 	int32_t		message number
	// 	int32_t		how many more messages follow
	// 	byte_t[]	message
	// }
	//
	// The message is on the wire either way - the count only says
	// whether another packet is coming after this one.

	uint32_t	byteswritten=0;

	opcode=op_send;

	return writeInt(opcode,"response op code",&byteswritten) &&
		writeInt(reqhandle,"request handle",&byteswritten) &&
		writeInt(incarnation,"incarnation",&byteswritten) &&
		writeInt(trhandle,"transaction handle",&byteswritten) &&
		writeInt(req->outmsg,"message number",&byteswritten) &&
		writeInt(msgcount,"message count",&byteswritten) &&
		writeRequestMessage(cursor,req,slots,&byteswritten);
}

bool sqlrprotocol_firebird::writeRequestMessage(sqlrservercursor *cursor,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrslot *slots,
					uint32_t *byteswritten) {

	// A request message is never packed, and carries no null indicator
	// of its own - the request declared an item for every value it
	// assigns, indicators included, so each item goes on the wire in
	// full and in order.

	sqlrfirebirdfield	*fields=req->msgfields[req->outmsg];
	uint16_t		fieldcount=req->msgfieldcount[req->outmsg];

	uint32_t	colcount=cont->colCount(cursor);

	for (uint16_t i=0; i<fieldcount; i++) {

		const sqlrfirebirdblrslot	*slot=&slots[i];

		const char	*value=NULL;
		uint64_t	valuesize=0;
		bool		lob=false;
		bool		null=true;
		uint32_t	col=0;

		if (slot->literal) {
			value=slot->literal;
			valuesize=charstring::getLength(slot->literal);
			null=false;
		} else if (slot->column>=0 &&
				(uint32_t)slot->column<colcount) {
			col=(uint32_t)slot->column;
			if (!cont->getField(cursor,col,&value,
						&valuesize,&lob,&null)) {
				return false;
			}
		} else if (slot->indicator>=0 &&
				(uint32_t)slot->indicator<colcount) {
			// an indicator item carries -1 for a null value and
			// 0 for anything else, rather than a value of its own
			const char	*indvalue=NULL;
			uint64_t	indvaluesize=0;
			bool		indlob=false;
			bool		indnull=false;
			if (!cont->getField(cursor,
					(uint32_t)slot->indicator,
					&indvalue,&indvaluesize,
					&indlob,&indnull)) {
				return false;
			}
			value=(indnull)?"-1":"0";
			valuesize=charstring::getLength(value);
			null=false;
		}

		if (!writeField(cursor,col,&fields[i],value,valuesize,
					lob,null,byteswritten)) {
			return false;
		}
	}

	return true;
}

bool sqlrprotocol_firebird::readRequestMessage(sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					sqlrfirebirdvalue *values,
					uint32_t *bytesread) {

	// the format a request message goes out in is the format it comes in
	// in - see writeRequestMessage()
	for (uint16_t i=0; i<fieldcount; i++) {
		if (!readMessageValue(&fields[i],NULL,i,false,
					&values[i],bytesread)) {
			return false;
		}
	}
	return true;
}

bool sqlrprotocol_firebird::infoSql() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	int32_t		incarnation
	// 	int32_t		requested sql info items length
	// 	byte_t[]	requested sql info items
	// 	int32_t		response buffer length
	// }

	debugStart("info sql");

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	incarnation;
	if (!readInt(&incarnation,"incarnation",&bytesread)) {
		return false;
	}

	uint32_t	itemslen;
	byte_t		*items;
	if (!readBuffer(&items,&itemslen,
			"requested sql info items",&bytesread)) {
		return false;
	}

	if (!readInt(&respbufferlen,"response buffer length",&bytesread)) {
		delete[] items;
		return false;
	}
	fixupRespBufferLen();

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->prepared) {
		delete[] items;
		return errorResponse("info sql response",isc_bad_stmt_handle);
	}

	respbuffer.clear();
	describeBinds(cursor,stmt,items,itemslen);
	buildSqlInfo(cursor,stmt,items,itemslen);

	delete[] items;

	successStatusVector();

	return genericResponse("info sql response",
				stmthandle,0,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorstr,
				statusvectorlen);
}

sqlrfirebirdblob *sqlrprotocol_firebird::newBlob() {

	// an id of 0 means "no blob", so the counter skips it on a wrap
	nextblobid++;
	if (!nextblobid) {
		nextblobid++;
	}

	sqlrfirebirdblob	*blob=new sqlrfirebirdblob;
	blob->id=nextblobid;
	blob->handle=0;
	blob->iswrite=false;
	blob->segcount=0;
	blob->maxseglength=0;
	blob->readpos=0;
	blob->readseg=0;
	blob->readsegstart=0;
	blob->isstream=false;
	blobs.append(blob);
	return blob;
}

uint32_t sqlrprotocol_firebird::newBlobHandle() {

	// a response carries the handle in a 16-bit field that firebird writes
	// with xdr_short, which sign extends, so a handle at 0x8000 or above
	// comes back as 0xffff8000 or higher and stops matching
	// (0 means "no blob", and a handle an open blob is using can't be
	// handed out twice)
	for (uint32_t i=0; i<0x7fff; i++) {
		nextblobhandle=(nextblobhandle+1)&0x7fff;
		if (!nextblobhandle) {
			nextblobhandle=1;
		}
		if (!getBlobByHandle(nextblobhandle)) {
			return nextblobhandle;
		}
	}
	return 0;
}

sqlrfirebirdblob *sqlrprotocol_firebird::getBlobById(uint32_t high,
							uint32_t low) {

	// the ids the module hands out all have a zero high word - see
	// newBlob()
	if (high || !low) {
		return NULL;
	}
	for (listnode< sqlrfirebirdblob * > *node=blobs.getFirst();
						node; node=node->getNext()) {
		if (node->getValue()->id==low) {
			return node->getValue();
		}
	}
	return NULL;
}

sqlrfirebirdblob *sqlrprotocol_firebird::getBlobByHandle(
						uint32_t blobhandle) {
	if (!blobhandle) {
		return NULL;
	}
	for (listnode< sqlrfirebirdblob * > *node=blobs.getFirst();
						node; node=node->getNext()) {
		if (node->getValue()->handle==blobhandle) {
			return node->getValue();
		}
	}
	return NULL;
}

void sqlrprotocol_firebird::removeBlob(sqlrfirebirdblob *blob) {
	for (listnode< sqlrfirebirdblob * > *node=blobs.getFirst();
						node; node=node->getNext()) {
		if (node->getValue()==blob) {
			blobbytes=blobbytes-blob->data.getSize();
			delete blob;
			blobs.remove(node);
			return;
		}
	}
}

void sqlrprotocol_firebird::clearBlobs() {
	for (listnode< sqlrfirebirdblob * > *node=blobs.getFirst();
						node; node=node->getNext()) {
		delete node->getValue();
	}
	blobs.clear();
	blobbytes=0;
}

void sqlrprotocol_firebird::trimBlobs() {

	// drop the oldest blobs nothing is reading and nothing can still
	// bind, until the session is back under budget
	// (their ids stop resolving, so an over-budget client gets an error
	// rather than an out-of-memory.  the ceiling is on how many pile up,
	// not on how big one is.)
	listnode< sqlrfirebirdblob * >	*node=blobs.getFirst();
	while (node && blobbytes>MAX_BLOB_BUFFER) {
		listnode< sqlrfirebirdblob * >	*next=node->getNext();
		sqlrfirebirdblob		*blob=node->getValue();
		if (!blob->handle && !blob->iswrite) {
			blobbytes=blobbytes-blob->data.getSize();
			delete blob;
			blobs.remove(node);
		}
		node=next;
	}
}

void sqlrprotocol_firebird::parseBpb(const byte_t *bpb,
					uint32_t bpblen,
					sqlrfirebirdblob *blob) {

	// a version byte, then items of an item byte, a length byte and that
	// many value bytes, little-endian
	// (isc_bpb_version1 and isc_bpb_source_type are both 1, so the walk
	// starts after the version byte rather than at it)
	if (!bpb || bpblen<2) {
		return;
	}

	const byte_t	*p=bpb+1;
	const byte_t	*end=bpb+bpblen;
	while (p+2<=end) {

		byte_t	item=*p;
		p++;

		byte_t	len=*p;
		p++;

		if (p+len>end) {
			break;
		}

		uint32_t	value=0;
		for (byte_t i=0; i<len && i<4; i++) {
			value=value|(((uint32_t)p[i])<<(8*i));
		}

		switch (item) {
			case isc_bpb_type:
				blob->isstream=
					((value&isc_bpb_type_stream)!=0);
				break;
			default:
				break;
		}

		p=p+len;
	}
}

void sqlrprotocol_firebird::appendBlobSegment(sqlrfirebirdblob *blob,
						const byte_t *value,
						uint32_t valuelen) {
	if (!value || !valuelen) {
		return;
	}
	blob->data.append(value,valuelen);
	blob->seglengths.append((const byte_t *)&valuelen,sizeof(valuelen));
	blob->segcount++;
	if (valuelen>blob->maxseglength) {
		blob->maxseglength=valuelen;
	}
	blobbytes=blobbytes+valuelen;
}

void sqlrprotocol_firebird::rewindBlob(sqlrfirebirdblob *blob,
						uint64_t position) {

	// turn a byte position into the segment it lands in and where that
	// segment starts
	const uint32_t	*seglengths=
			(const uint32_t *)blob->seglengths.getBuffer();

	blob->readpos=position;
	blob->readseg=blob->segcount;
	blob->readsegstart=blob->data.getSize();

	uint64_t	start=0;
	for (uint32_t i=0; i<blob->segcount; i++) {
		if (position<start+seglengths[i]) {
			blob->readseg=i;
			blob->readsegstart=start;
			return;
		}
		start=start+seglengths[i];
	}
}

void sqlrprotocol_firebird::bufferBlob(sqlrservercursor *cursor,
					uint32_t col,
					const char *value,
					uint64_t valuesize,
					bool lob,
					uint32_t *id) {

	trimBlobs();

	sqlrfirebirdblob	*blob=newBlob();
	*id=blob->id;

	// a column the backend handed back whole is one segment
	if (!lob) {
		appendBlobSegment(blob,(const byte_t *)value,
			(valuesize>0xffffffffULL)?0xffffffffU:
						(uint32_t)valuesize);
		return;
	}

	if (!lobbuffer) {
		lobbuffer=new char[BLOB_SEGMENT_SIZE*MAX_BYTES_PER_CHAR];
	}

	// getLobFieldLength() is what opens the lob, so it has to be called
	// even though the length it answers isn't used
	uint64_t	loblength=0;
	cont->getLobFieldLength(cursor,col,&loblength);

	// each read comes back at a segment boundary, because the firebird
	// backend's read loop stops short when a stored segment ends, so the
	// client reads a blob back segmented the way the backend stored it
	uint64_t	offset=0;
	for (;;) {
		uint64_t	charsread=0;
		if (!cont->getLobFieldSegment(cursor,col,lobbuffer,
					BLOB_SEGMENT_SIZE*MAX_BYTES_PER_CHAR,
					offset,BLOB_SEGMENT_SIZE,&charsread) ||
					!charsread) {
			break;
		}
		if (charsread>BLOB_SEGMENT_SIZE*MAX_BYTES_PER_CHAR) {
			charsread=BLOB_SEGMENT_SIZE*MAX_BYTES_PER_CHAR;
		}
		appendBlobSegment(blob,(const byte_t *)lobbuffer,
						(uint32_t)charsread);
		offset=offset+charsread;
	}

	cont->closeLobField(cursor,col);
}

sqlrfirebirdarray *sqlrprotocol_firebird::newArray() {

	// an id of 0 means "no array", so the counter skips it on a wrap
	nextarrayid++;
	if (!nextarrayid) {
		nextarrayid++;
	}

	sqlrfirebirdarray	*array=new sqlrfirebirdarray;
	array->id=nextarrayid;
	array->elementtype=0;
	array->elementscale=0;
	array->elementsize=0;
	array->elementcount=0;
	array->dimensions=0;
	for (uint16_t i=0; i<SQLRFIREBIRDSDL_MAX_DIMENSIONS; i++) {
		array->lower[i]=0;
		array->upper[i]=0;
	}
	arrays.append(array);
	return array;
}

sqlrfirebirdarray *sqlrprotocol_firebird::getArrayById(uint32_t high,
							uint32_t low) {

	// the ids the module hands out all have the same high word - see
	// newArray() and ARRAY_ID_HIGH
	if (high!=ARRAY_ID_HIGH || !low) {
		return NULL;
	}
	for (listnode< sqlrfirebirdarray * > *node=arrays.getFirst();
						node; node=node->getNext()) {
		if (node->getValue()->id==low) {
			return node->getValue();
		}
	}
	return NULL;
}

void sqlrprotocol_firebird::clearArrays() {
	for (listnode< sqlrfirebirdarray * > *node=arrays.getFirst();
						node; node=node->getNext()) {
		delete node->getValue();
	}
	arrays.clear();
	arraybytes=0;
}

void sqlrprotocol_firebird::trimArrays() {

	// drop the oldest arrays until the session is back under budget
	// (their ids stop resolving, so an over-budget client gets an error
	// rather than an out-of-memory - the same bargain trimBlobs() makes)
	listnode< sqlrfirebirdarray * >	*node=arrays.getFirst();
	while (node && arraybytes>MAX_ARRAY_BUFFER) {
		listnode< sqlrfirebirdarray * >	*next=node->getNext();
		sqlrfirebirdarray		*array=node->getValue();
		arraybytes=arraybytes-array->data.getSize();
		delete array;
		arrays.remove(node);
		node=next;
	}
}

uint32_t sqlrprotocol_firebird::arrayElementSize(byte_t blrtype,
						uint16_t length) {

	// How many bytes one element of an array takes up.  This is what
	// firebird's sdl_desc() (common/sdl.cpp) works out from the same blr
	// type and length, and it's the stride both ends of the wire use.
	// Note that a varying element is described there as a cstring of
	// length+2 bytes rather than as a varying, so it's stored
	// null-terminated rather than length-prefixed.
	// (0 means an element type this module can't stride over.)
	switch (blrtype) {
		case blr_text:
		case blr_text2:
		case blr_cstring:
		case blr_cstring2:
			return length;
		case blr_varying:
		case blr_varying2:
			return length+sizeof(uint16_t);
		case blr_bool:
			return 1;
		case blr_short:
			return 2;
		case blr_long:
		case blr_float:
		case blr_sql_date:
		case blr_sql_time:
			return 4;
		case blr_int64:
		case blr_quad:
		case blr_double:
		case blr_d_float:
		case blr_timestamp:
			return 8;
		default:
			return 0;
	}
}

void sqlrprotocol_firebird::bufferArray(sqlrservercursor *cursor,
					uint32_t col,
					uint32_t *id) {

	trimArrays();

	sqlrfirebirdarray	*array=newArray();
	*id=array->id;

	// the descriptor says how the array is shaped and how wide an element
	// is.  It's opaque as far as the server API is concerned, and only a
	// firebird backend has one to give, so a descriptor that isn't an
	// ISC_ARRAY_DESC leaves the bounds unknown - see getSlice(), which
	// then has to take the client's slice as covering the whole array.
	const unsigned char	*descriptor=NULL;
	uint64_t		descriptorsize=0;
	if (!cont->getArrayFieldDescriptor(cursor,col,
					&descriptor,&descriptorsize) ||
		!descriptor ||
		descriptorsize!=sizeof(sqlrfirebirdarraydesc)) {
		cont->closeArrayField(cursor,col);
		return;
	}

	const sqlrfirebirdarraydesc	*desc=
			(const sqlrfirebirdarraydesc *)descriptor;

	if (desc->dimensions<1 ||
		desc->dimensions>(int16_t)SQLRFIREBIRDSDL_MAX_DIMENSIONS) {
		cont->closeArrayField(cursor,col);
		return;
	}

	array->elementtype=desc->dtype;
	array->elementscale=(int8_t)desc->scale;
	array->elementsize=arrayElementSize(desc->dtype,desc->length);
	if (!array->elementsize) {
		cont->closeArrayField(cursor,col);
		return;
	}

	array->dimensions=(uint16_t)desc->dimensions;
	for (uint16_t i=0; i<array->dimensions; i++) {
		array->lower[i]=desc->bounds[i].lower;
		array->upper[i]=desc->bounds[i].upper;
	}

	// pull the elements over in batches
	// (the fetch loop has moved on to the next row by the time the client
	// asks for a slice, so the whole array has to be buffered here - the
	// same reason bufferBlob() buffers a blob whole)
	// cap the batch so a wide element (eg. a large varchar) can't blow the
	// staging buffer out past what the whole array is allowed to occupy
	uint64_t	batchelements=ARRAY_SLICE_ELEMENTS;
	if (batchelements*array->elementsize>MAX_ARRAY_BUFFER) {
		batchelements=MAX_ARRAY_BUFFER/array->elementsize;
	}
	if (!batchelements) {
		batchelements=1;
	}
	uint64_t	buffersize=batchelements*array->elementsize;
	char		*buffer=new char[buffersize];
	uint64_t	offset=0;
	for (;;) {
		uint64_t	elementsread=0;
		if (!cont->getArrayFieldSlice(cursor,col,buffer,buffersize,
						offset,batchelements,
						&elementsread) ||
						!elementsread) {
			break;
		}
		uint64_t	bytes=elementsread*array->elementsize;
		array->data.append((const byte_t *)buffer,(size_t)bytes);
		arraybytes=arraybytes+bytes;
		offset=offset+elementsread;
		if (arraybytes>MAX_ARRAY_BUFFER) {
			break;
		}
	}
	delete[] buffer;

	array->elementcount=offset;

	cont->closeArrayField(cursor,col);
}

bool sqlrprotocol_firebird::createBlob() {
	return createBlobCommon("create blob",false);
}

bool sqlrprotocol_firebird::createBlob2() {
	return createBlobCommon("create blob2",true);
}

bool sqlrprotocol_firebird::createBlobCommon(const char *title, bool hasbpb) {

	// request packet data structure:
	//
	// data {
	// 	int32_t		bpb length		(op_create_blob2 only)
	// 	byte_t[]	bpb			(op_create_blob2 only)
	// 	byte_t[]	bpb padding		(op_create_blob2 only)
	// 	int32_t		transaction handle
	// 	int32_t		blob id, high word
	// 	int32_t		blob id, low word
	// }
	//
	// (the bpb comes in front of the transaction handle, not after it)

	debugStart(title);

	uint32_t	bytesread=0;

	uint32_t	bpblen=0;
	byte_t		*bpb=NULL;
	if (hasbpb && !readBuffer(&bpb,&bpblen,"bpb",&bytesread)) {
		return false;
	}

	uint32_t	clienttrhandle;
	uint32_t	high;
	uint32_t	low;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread) ||
		!readInt(&high,"blob id high word",&bytesread) ||
		!readInt(&low,"blob id low word",&bytesread)) {
		delete[] bpb;
		return false;
	}

	debugEnd();

	uint32_t	blobhandle=newBlobHandle();
	if (!blobhandle) {
		delete[] bpb;
		return errorResponse(title,isc_bad_segstr_handle);
	}

	// the id the client sent is only meaningful to an open - a create
	// answers one of the module's own
	trimBlobs();
	sqlrfirebirdblob	*blob=newBlob();
	blob->iswrite=true;
	blob->handle=blobhandle;
	parseBpb(bpb,bpblen,blob);

	delete[] bpb;

	successStatusVector();

	return genericResponse(title,
				blob->handle,(uint64_t)blob->id,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::openBlob() {
	return openBlobCommon("open blob",false);
}

bool sqlrprotocol_firebird::openBlob2() {
	return openBlobCommon("open blob2",true);
}

bool sqlrprotocol_firebird::openBlobCommon(const char *title, bool hasbpb) {

	// request packet data structure:
	//
	// data {
	// 	int32_t		bpb length		(op_open_blob2 only)
	// 	byte_t[]	bpb			(op_open_blob2 only)
	// 	byte_t[]	bpb padding		(op_open_blob2 only)
	// 	int32_t		transaction handle
	// 	int32_t		blob id, high word
	// 	int32_t		blob id, low word
	// }

	debugStart(title);

	uint32_t	bytesread=0;

	uint32_t	bpblen=0;
	byte_t		*bpb=NULL;
	if (hasbpb && !readBuffer(&bpb,&bpblen,"bpb",&bytesread)) {
		return false;
	}

	uint32_t	clienttrhandle;
	uint32_t	high;
	uint32_t	low;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread) ||
		!readInt(&high,"blob id high word",&bytesread) ||
		!readInt(&low,"blob id low word",&bytesread)) {
		delete[] bpb;
		return false;
	}

	debugEnd();

	delete[] bpb;

	sqlrfirebirdblob	*blob=getBlobById(high,low);
	if (!blob) {
		return errorResponse(title,isc_bad_segstr_id);
	}

	// keep the handle an already-open blob has rather than taking a second
	// one - abandoning the first would put its number back in the pool,
	// and a client still holding it would reach whichever blob got it next
	uint32_t	blobhandle=blob->handle;
	if (!blobhandle) {
		blobhandle=newBlobHandle();
		if (!blobhandle) {
			return errorResponse(title,isc_bad_segstr_handle);
		}
	}

	// reading starts over at the front
	blob->handle=blobhandle;
	rewindBlob(blob,0);

	successStatusVector();

	return genericResponse(title,
				blob->handle,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::getSegment() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		blob handle
	// 	int32_t		how many bytes the client will take
	// 	int32_t		segment buffer length	(always 0 here)
	// 	byte_t[]	segment buffer		(always empty here)
	// }

	debugStart("get segment");

	uint32_t	bytesread=0;

	uint32_t	blobhandle;
	uint32_t	buflen;
	if (!readInt(&blobhandle,"blob handle",&bytesread) ||
		!readInt(&buflen,"buffer length",&bytesread)) {
		return false;
	}

	uint32_t	seglen=0;
	byte_t		*seg=NULL;
	if (!readBuffer(&seg,&seglen,"segment buffer",&bytesread)) {
		return false;
	}
	delete[] seg;

	debugEnd();

	// the length is a USHORT that went over the wire sign-extended, and
	// firebird's own length fixup doesn't run on this one, so a client
	// asking for 32768 or more arrives negative
	// (this fold is unconditional on purpose - it isn't the
	// sign-extension-only rule that foldSignExtendedLength() applies)
	buflen=buflen&0xffff;

	sqlrfirebirdblob	*blob=getBlobByHandle(blobhandle);
	if (!blob) {
		return errorResponse("get segment response",
					isc_bad_segstr_handle);
	}

	// pack whole segments into the client's buffer, each behind a 2-byte
	// little-endian length, until the buffer fills or the blob runs out
	// (a segment only part of which fit says so, and the client renders
	// that as isc_segment)
	const byte_t	*data=blob->data.getBuffer();
	const uint32_t	*seglengths=
			(const uint32_t *)blob->seglengths.getBuffer();

	respbuffer.clear();

	uint32_t	written=0;
	uint32_t	state=BLOB_MORE;
	for (;;) {

		if (blob->readseg>=blob->segcount) {
			state=BLOB_EOF;
			break;
		}

		// 2 for the length word, and at least 1 byte of segment
		if (written+3>buflen) {
			break;
		}

		uint32_t	available=seglengths[blob->readseg]-
			(uint32_t)(blob->readpos-blob->readsegstart);
		uint32_t	space=buflen-written-2;
		uint32_t	count=(available<space)?available:space;

		writeLE(&respbuffer,(uint16_t)count);
		write(&respbuffer,data+blob->readpos,count);
		written=written+2+count;

		blob->readpos=blob->readpos+count;

		if (count<available) {
			state=BLOB_PARTIAL_SEGMENT;
			break;
		}

		blob->readsegstart=blob->readsegstart+
					seglengths[blob->readseg];
		blob->readseg++;
	}

	successStatusVector();

	return genericResponse("get segment response",
				state,0,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::putSegment() {
	return putSegmentCommon("put segment",false);
}

bool sqlrprotocol_firebird::batchSegment() {
	return putSegmentCommon("batch segment",true);
}

bool sqlrprotocol_firebird::putSegmentCommon(const char *title, bool batch) {

	// request packet data structure:
	//
	// data {
	// 	int32_t		blob handle
	// 	int32_t		segment length
	// 	int32_t		segment buffer length
	// 	byte_t[]	segment buffer
	// 	byte_t[]	segment buffer padding
	// }
	//
	// op_put_segment's buffer is one segment's raw bytes.
	// op_batch_segments packs several, each behind a 2-byte little-endian
	// length, with no padding between them.

	debugStart(title);

	uint32_t	bytesread=0;

	uint32_t	blobhandle;
	uint32_t	seglen;
	if (!readInt(&blobhandle,"blob handle",&bytesread) ||
		!readInt(&seglen,"segment length",&bytesread)) {
		return false;
	}

	uint32_t	buflen=0;
	byte_t		*buf=NULL;
	if (!readBuffer(&buf,&buflen,"segment buffer",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrfirebirdblob	*blob=getBlobByHandle(blobhandle);
	if (!blob) {
		delete[] buf;
		return errorResponse(title,isc_bad_segstr_handle);
	}

	// enforce the ceiling here, since trimBlobs() will never reclaim a
	// blob the client is still building, and a client that puts segments
	// in a loop would grow the connection process without limit
	trimBlobs();
	if (blobbytes>MAX_BLOB_BUFFER) {
		delete[] buf;
		return errorResponse(title,isc_imp_exc,"54000",-904,
					"Blob buffer limit exceeded",26);
	}

	if (batch) {
		uint32_t	pos=0;
		while (pos+2<=buflen) {
			uint32_t	len=buf[pos]|(((uint32_t)buf[pos+1])<<8);
			pos=pos+2;
			if (pos+len>buflen) {
				break;
			}
			appendBlobSegment(blob,buf+pos,len);
			pos=pos+len;
		}
	} else {
		appendBlobSegment(blob,buf,buflen);
	}

	delete[] buf;

	successStatusVector();

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::seekBlob() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		blob handle
	// 	int32_t		mode
	// 	int32_t		offset
	// }

	debugStart("seek blob");

	uint32_t	bytesread=0;

	uint32_t	blobhandle;
	uint32_t	mode;
	uint32_t	offset;
	if (!readInt(&blobhandle,"blob handle",&bytesread) ||
		!readInt(&mode,"mode",&bytesread) ||
		!readInt(&offset,"offset",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrfirebirdblob	*blob=getBlobByHandle(blobhandle);
	if (!blob) {
		return errorResponse("seek blob response",
					isc_bad_segstr_handle);
	}

	// firebird refuses a seek on a segmented blob, but the module holds
	// every blob whole, so it can serve one either way
	int64_t	total=(int64_t)blob->data.getSize();
	int64_t	position=(int32_t)offset;
	if (mode==BLOB_SEEK_END) {
		position=total+position;
	} else if (mode==BLOB_SEEK_RELATIVE) {
		position=(int64_t)blob->readpos+position;
	}
	if (position<0) {
		position=0;
	}
	if (position>total) {
		position=total;
	}

	rewindBlob(blob,(uint64_t)position);

	successStatusVector();

	// the new position goes in the low word of the blob id, not in the
	// object handle - the published wire document has this wrong
	return genericResponse("seek blob response",
				0,(uint64_t)position,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::cancelBlob() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		blob handle
	// }

	debugStart("cancel blob");

	uint32_t	bytesread=0;

	uint32_t	blobhandle;
	if (!readInt(&blobhandle,"blob handle",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrfirebirdblob	*blob=getBlobByHandle(blobhandle);
	if (!blob) {
		return errorResponse("cancel blob response",
					isc_bad_segstr_handle);
	}

	// a cancelled blob is discarded, id and all
	removeBlob(blob);

	successStatusVector();

	return genericResponse("cancel blob response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::closeBlob() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		blob handle
	// }

	debugStart("close blob");

	uint32_t	bytesread=0;

	uint32_t	blobhandle;
	if (!readInt(&blobhandle,"blob handle",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrfirebirdblob	*blob=getBlobByHandle(blobhandle);
	if (!blob) {
		return errorResponse("close blob response",
					isc_bad_segstr_handle);
	}

	// the bytes outlive the handle - a client binds a blob's id into an
	// insert only after closing it, and the id stays good until the
	// transaction that made it ends
	blob->handle=0;
	rewindBlob(blob,0);

	successStatusVector();

	return genericResponse("close blob response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::infoBlob() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		blob handle
	// 	int32_t		incarnation
	// 	int32_t		requested blob info items length
	// 	byte_t[]	requested blob info items
	// 	int32_t		response buffer length
	// }

	debugStart("info blob");

	uint32_t	bytesread=0;

	uint32_t	blobhandle;
	uint32_t	incarnation;
	if (!readInt(&blobhandle,"blob handle",&bytesread) ||
		!readInt(&incarnation,"incarnation",&bytesread)) {
		return false;
	}

	uint32_t	itemslen;
	byte_t		*items;
	if (!readBuffer(&items,&itemslen,
			"requested blob info items",&bytesread)) {
		return false;
	}

	if (!readInt(&respbufferlen,"response buffer length",&bytesread)) {
		delete[] items;
		return false;
	}
	fixupRespBufferLen();

	debugEnd();

	sqlrfirebirdblob	*blob=getBlobByHandle(blobhandle);
	if (!blob) {
		delete[] items;
		return errorResponse("info blob response",
					isc_bad_segstr_handle);
	}

	respbuffer.clear();

	bool	fits=true;
	for (uint32_t i=0; i<itemslen && fits; i++) {

		byte_t	item=items[i];

		if (getDebug()) {
			stdoutput.printf("	item: %d\n",item);
		}

		if (item==isc_info_end) {
			break;
		}

		switch (item) {
			case isc_info_blob_num_segments:
				fits=appendInfoInt(item,blob->segcount);
				break;
			case isc_info_blob_max_segment:
				fits=appendInfoInt(item,blob->maxseglength);
				break;
			case isc_info_blob_total_length:
				fits=appendInfoInt(item,
					(uint32_t)blob->data.getSize());
				break;
			case isc_info_blob_type:
				fits=appendInfoByte(item,
						(byte_t)
						((blob->isstream)?1:0));
				break;
			default:
				fits=appendInfoError(item);
				break;
		}
	}

	delete[] items;

	if (fits && respbuffer.getSize()<respbufferlen) {
		write(&respbuffer,(byte_t)isc_info_end);
	}

	successStatusVector();

	return genericResponse("info blob response",
				blobhandle,0,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::infoBatch() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	int32_t		incarnation
	// 	int32_t		requested batch info items length
	// 	byte_t[]	requested batch info items
	// 	int32_t		response buffer length
	// }
	//
	// (a batch has no handle of its own - see op_batch_create - so this
	// is the statement handle, the same as every other op_batch_* op)

	debugStart("info batch");

	const char	*title="info batch response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	uint32_t	incarnation;
	if (!readInt(&stmthandle,"statement handle",&bytesread) ||
		!readInt(&incarnation,"incarnation",&bytesread)) {
		return false;
	}

	uint32_t	itemslen;
	byte_t		*items;
	if (!readBuffer(&items,&itemslen,
			"requested batch info items",&bytesread)) {
		return false;
	}

	if (!readInt(&respbufferlen,"response buffer length",&bytesread)) {
		delete[] items;
		return false;
	}
	fixupRespBufferLen();

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->batch.open) {
		delete[] items;
		return errorResponse(title,isc_bad_batch_handle);
	}

	sqlrfirebirdbatch	*batch=&stmt->batch;

	respbuffer.clear();

	bool	fits=true;
	for (uint32_t i=0; i<itemslen && fits; i++) {

		byte_t	item=items[i];

		if (getDebug()) {
			stdoutput.printf("	item: %d\n",item);
		}

		if (item==isc_info_end) {
			break;
		}

		switch (item) {
			case BATCH_inf_buffer_bytes_size:
				fits=appendInfoInt(item,batch->buffersize);
				break;
			case BATCH_inf_data_size:
				fits=appendInfoInt(item,
						(uint32_t)batch->queuedbytes);
				break;
			case BATCH_inf_element_alignment:
				fits=appendInfoInt(item,
						BATCH_blob_stream_align);
				break;
			case BATCH_inf_blob_header:
				fits=appendInfoInt(item,
						BATCH_blob_hdr_size);
				break;
			// blob bytes queued aren't tracked separately from
			// the rest of a message's data (see queuedbytes) -
			// nothing backs this item
			case BATCH_inf_blobs_size:
			default:
				fits=appendInfoError(item);
				break;
		}
	}

	delete[] items;

	if (fits && respbuffer.getSize()<respbufferlen) {
		write(&respbuffer,(byte_t)isc_info_end);
	}

	successStatusVector();

	return genericResponse(title,
				stmthandle,0,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorstr,
				statusvectorlen);
}

// renders a scaled integer the way the firebird connection module's
// firebirdFormatScaledInt64() does, so an array that goes out through one
// and comes back through the other reads the same both ways
static void appendScaledInt64(stringbuffer *output, int64_t v, int8_t scale) {

	int16_t	digits=-scale;

	// 10^10 already overflows an int, so the divisor is built with
	// integer math rather than with pow()
	int64_t	p=1;
	for (int16_t i=0; i<digits; i++) {
		p*=10;
	}

	// Integer division truncates toward zero and the remainder carries
	// the sign, so formatting the halves separately would put a sign on
	// each of them, and lose it entirely when the integer part is zero.
	int64_t	whole=v/p;
	int64_t	frac=v%p;
	if (whole<0) {
		whole=-whole;
	}
	if (frac<0) {
		frac=-frac;
	}
	if (v<0) {
		output->append('-');
	}
	output->append(whole);
	output->append('.');

	// zero pad the fraction out to the scale
	int64_t	q=p/10;
	while (q>1 && frac<q) {
		output->append('0');
		q/=10;
	}
	output->append(frac);
}

bool sqlrprotocol_firebird::appendArrayElement(stringbuffer *output,
						byte_t blrtype,
						int8_t scale,
						const byte_t *element,
						uint32_t elementsize) {

	// One element of an array, rendered the way the firebird connection
	// module renders one - see firebirdAppendArrayElement() there.  The
	// two have to agree, since this is what gets bound back to the
	// backend as an array bind, and that is what parses it.
	// (nothing here is guaranteed to be aligned - the elements are
	// packed - so each one is copied into a local before it's read)
	char	buffer[64];

	switch (blrtype) {

		case blr_short:
			{
			int16_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			if (scale) {
				appendScaledInt64(output,(int64_t)v,scale);
			} else {
				output->append((int64_t)v);
			}
			return true;
			}

		case blr_long:
			{
			int32_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			if (scale) {
				appendScaledInt64(output,(int64_t)v,scale);
			} else {
				output->append((int64_t)v);
			}
			return true;
			}

		case blr_int64:
			{
			int64_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			if (scale) {
				appendScaledInt64(output,v,scale);
			} else {
				output->append(v);
			}
			return true;
			}

		case blr_float:
			{
			float	v=0.0;
			bytestring::copy(&v,element,sizeof(v));
			charstring::printf(buffer,sizeof(buffer),
						"%.4f",(double)v);
			output->append(buffer);
			return true;
			}

		case blr_double:
		case blr_d_float:
			{
			double	v=0.0;
			bytestring::copy(&v,element,sizeof(v));
			charstring::printf(buffer,sizeof(buffer),"%.4f",v);
			output->append(buffer);
			return true;
			}

		case blr_bool:
			output->append((element[0])?'1':'0');
			return true;

		case blr_sql_date:
			{
			uint32_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			int16_t	year=1;
			int16_t	month=1;
			int16_t	day=1;
			decodeDate(v,&year,&month,&day);
			charstring::printf(buffer,sizeof(buffer),
						"'%d-%02d-%02d'",
						year,month,day);
			output->append(buffer);
			return true;
			}

		case blr_sql_time:
			{
			uint32_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			int16_t	hour=0;
			int16_t	minute=0;
			int16_t	second=0;
			int32_t	microsecond=0;
			decodeTime(v,&hour,&minute,&second,&microsecond);
			charstring::printf(buffer,sizeof(buffer),
						"'%02d:%02d:%02d'",
						hour,minute,second);
			output->append(buffer);
			return true;
			}

		case blr_timestamp:
			{
			// a timestamp element is a date and a time, in that
			// order - see writeSliceElement()
			uint32_t	d=0;
			uint32_t	t=0;
			bytestring::copy(&d,element,sizeof(d));
			bytestring::copy(&t,element+sizeof(d),sizeof(t));
			int16_t	year=1;
			int16_t	month=1;
			int16_t	day=1;
			int16_t	hour=0;
			int16_t	minute=0;
			int16_t	second=0;
			int32_t	microsecond=0;
			decodeDate(d,&year,&month,&day);
			decodeTime(t,&hour,&minute,&second,&microsecond);
			charstring::printf(buffer,sizeof(buffer),
					"'%d-%02d-%02d %02d:%02d:%02d'",
					year,month,day,hour,minute,second);
			output->append(buffer);
			return true;
			}

		case blr_text:
		case blr_text2:
			{
			// a text element is blank padded out to its width
			uint32_t	len=elementsize;
			while (len && element[len-1]==' ') {
				len--;
			}
			output->append('\'');
			output->append((const char *)element,len);
			output->append('\'');
			return true;
			}

		case blr_cstring:
		case blr_cstring2:
		case blr_varying:
		case blr_varying2:
			{
			// firebird's sdl_desc() describes a varying array
			// element as a cstring, so both are stored
			// null-terminated inside the element's width
			uint32_t	len=0;
			while (len<elementsize && element[len]) {
				len++;
			}
			output->append('\'');
			output->append((const char *)element,len);
			output->append('\'');
			return true;
			}

		default:
			return false;
	}
}

bool sqlrprotocol_firebird::readSliceElement(byte_t blrtype,
						uint32_t elementsize,
						byte_t *element,
						uint32_t *bytesread) {

	// The inverse of writeSliceElement() - one element of a slice, read
	// off the wire in the encoding firebird's xdr_datum()
	// (common/xdr.cpp) uses, into the packed bytes an array is held as.
	// (the elements are packed, so nothing about them is guaranteed to
	// be aligned - each one is built in a local and copied out)

	switch (blrtype) {

		case blr_short:
			{
			uint32_t	v=0;
			if (!readInt(&v,"slice element",bytesread)) {
				return false;
			}
			int16_t	s=(int16_t)(int32_t)v;
			bytestring::copy(element,&s,sizeof(s));
			return true;
			}

		case blr_long:
		case blr_sql_date:
		case blr_sql_time:
		case blr_float:
			{
			uint32_t	v=0;
			if (!readInt(&v,"slice element",bytesread)) {
				return false;
			}
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}

		case blr_int64:
		case blr_double:
		case blr_d_float:
			{
			uint64_t	v=0;
			if (!readInt64(&v,"slice element",bytesread)) {
				return false;
			}
			bytestring::copy(element,&v,sizeof(v));
			return true;
			}

		case blr_quad:
		case blr_timestamp:
			{
			// two longs, high word first, which is the order
			// they go in in memory too
			uint32_t	high=0;
			uint32_t	low=0;
			if (!readInt(&high,"slice element",bytesread) ||
				!readInt(&low,"slice element",bytesread)) {
				return false;
			}
			bytestring::copy(element,&high,sizeof(high));
			bytestring::copy(element+sizeof(high),
						&low,sizeof(low));
			return true;
			}

		case blr_bool:
			return readOpaque(element,1,
					"slice element",bytesread);

		case blr_text:
		case blr_text2:
			return readOpaque(element,elementsize,
					"slice element",bytesread);

		case blr_cstring:
		case blr_cstring2:
		case blr_varying:
		case blr_varying2:
			{
			// a length, and that many bytes of a value that is
			// stored null-terminated inside the element's width
			uint32_t	len=0;
			if (!readInt(&len,"slice element length",bytesread)) {
				return false;
			}
			// the value has to fit inside the element's width
			// with room for the terminator - a longer one would
			// overrun the element and leave the rest of the
			// slice misaligned, so there's no reading on past it
			if (len>=elementsize) {
				return false;
			}
			if (!readOpaque(element,len,
					"slice element",bytesread)) {
				return false;
			}
			bytestring::zero(element+len,elementsize-len);
			return true;
			}

		default:
			return false;
	}
}

bool sqlrprotocol_firebird::drainSliceElements(byte_t blrtype,
						uint32_t elementsize,
						uint64_t elementcount,
						uint32_t *bytesread) {

	// read and discard elements the caller isn't keeping, so the socket
	// ends up where the client's slice length said it would - each
	// element still has to go through readSliceElement() since its wire
	// width isn't just elementsize (a short costs a full 4-byte xdr
	// slot, for instance)
	if (!elementcount) {
		return true;
	}
	byte_t	*discard=new byte_t[elementsize];
	bool	success=true;
	for (uint64_t i=0; i<elementcount; i++) {
		if (!readSliceElement(blrtype,elementsize,
					discard,bytesread)) {
			success=false;
			break;
		}
	}
	delete[] discard;
	return success;
}

bool sqlrprotocol_firebird::writeSliceElement(byte_t blrtype,
						uint32_t elementsize,
						const byte_t *element,
						uint32_t *byteswritten) {

	// One element of a slice, encoded the way firebird's xdr_datum()
	// (common/xdr.cpp) encodes it.  The module always accepts
	// arch_generic (see connect()), so the client keeps xdr on and reads
	// the elements one at a time rather than as raw bytes.
	// (nothing here is guaranteed to be aligned - the elements are
	// packed - so each one is copied into a local before it's read)
	switch (blrtype) {

		case blr_short:
			{
			int16_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			return writeInt((uint32_t)(int32_t)v,
					"slice element",byteswritten);
			}

		case blr_long:
		case blr_sql_date:
		case blr_sql_time:
		case blr_float:
			{
			uint32_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			return writeInt(v,"slice element",byteswritten);
			}

		case blr_int64:
		case blr_double:
		case blr_d_float:
			{
			uint64_t	v=0;
			bytestring::copy(&v,element,sizeof(v));
			return writeInt64(v,"slice element",byteswritten);
			}

		case blr_quad:
		case blr_timestamp:
			{
			// two longs, high word first, which is the order
			// they're already in in memory
			uint32_t	high=0;
			uint32_t	low=0;
			bytestring::copy(&high,element,sizeof(high));
			bytestring::copy(&low,element+sizeof(high),
						sizeof(low));
			return writeInt(high,"slice element",byteswritten) &&
				writeInt(low,"slice element",byteswritten);
			}

		case blr_bool:
			return writeOpaque(element,1,
					"slice element",byteswritten);

		case blr_text:
		case blr_text2:
			return writeOpaque(element,elementsize,
					"slice element",byteswritten);

		case blr_cstring:
		case blr_cstring2:
		case blr_varying:
		case blr_varying2:
			{
			// firebird's sdl_desc() describes a varying array
			// element as a cstring, so both go over as one - a
			// length, and that many bytes of a null-terminated
			// value
			uint32_t	len=0;
			while (len+1<elementsize && element[len]) {
				len++;
			}
			return writeInt(len,"slice element length",
						byteswritten) &&
				writeOpaque(element,len,
						"slice element",byteswritten);
			}

		default:
			return false;
	}
}

bool sqlrprotocol_firebird::getSlice() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// 	int32_t		array id, high word
	// 	int32_t		array id, low word
	// 	int32_t		how many bytes the client will take
	// 	int32_t		sdl length
	// 	byte_t[]	sdl
	// 	byte_t[]	sdl padding
	// 	int32_t		parameters length
	// 	byte_t[]	parameters
	// 	int32_t		slice length	(always 0 here)
	// }
	//
	// (firebird's P_SLC - see FB4 src/remote/protocol.cpp:590.  The
	// trailing slice is the request's own copy of the elements, which is
	// empty on a get and only carries anything on a put.)

	debugStart("get slice");

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	uint32_t	high;
	uint32_t	low;
	uint32_t	clientbufferlen;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread) ||
		!readInt(&high,"array id high word",&bytesread) ||
		!readInt(&low,"array id low word",&bytesread) ||
		!readInt(&clientbufferlen,"buffer length",&bytesread)) {
		return false;
	}

	uint32_t	sdllen=0;
	byte_t		*sdl=NULL;
	if (!readBuffer(&sdl,&sdllen,"sdl",&bytesread)) {
		return false;
	}

	// the parameters carry values for any expressions in the sdl, and
	// nothing that generates an sdl here puts an expression in one, so
	// this is read and dropped
	uint32_t	paramslen=0;
	byte_t		*params=NULL;
	if (!readBuffer(&params,&paramslen,"parameters",&bytesread)) {
		delete[] sdl;
		return false;
	}
	delete[] params;

	uint32_t	slicelen=0;
	if (!readInt(&slicelen,"slice length",&bytesread)) {
		delete[] sdl;
		return false;
	}

	debugEnd();

	sqlrfirebirdarray	*array=getArrayById(high,low);
	if (!array) {
		delete[] sdl;
		// firebird has no code of its own for a bad array id - its
		// own server reports one out of the same blob-id check
		return errorResponse("get slice response",
					isc_bad_segstr_id);
	}

	// work out what the client asked for
	sqlrfirebirdsdl	parsedsdl;
	if (!parsedsdl.parse(sdl,sdllen)) {
		if (getDebug()) {
			stdoutput.printf("	sdl parse failed: %s\n",
						parsedsdl.getError());
		}
		delete[] sdl;
		return errorResponse("get slice response",isc_invalid_sdl);
	}
	delete[] sdl;

	// the element the client is expecting has to be the one that was
	// buffered, or the bytes would be read at the wrong stride
	byte_t		elementtype=parsedsdl.getElementType();
	uint32_t	elementsize=arrayElementSize(elementtype,
						parsedsdl.getElementLength());
	if (!elementsize || elementsize!=array->elementsize) {
		return errorResponse("get slice response",isc_invalid_sdl);
	}

	uint64_t	count=parsedsdl.getElementCount();
	if (!count) {
		return errorResponse("get slice response",isc_invalid_sdl);
	}

	// never answer with more than the client said it would take -
	// firebird's own server caps it the same way, by only giving the
	// engine a buffer that big
	uint64_t	maxcount=clientbufferlen/elementsize;
	if (count>maxcount) {
		count=maxcount;
	}

	// gather the elements the slice asks for out of the buffered array
	const byte_t	*data=array->data.getBuffer();

	respbuffer.clear();

	for (uint64_t i=0; i<count; i++) {

		uint64_t	index=i;

		// A slice can be a sub-range of the array, and then its own
		// subscripts have to be mapped onto the array's.  When the
		// backend gave no bounds to map onto, the slice is taken as
		// covering the whole array, which is what a client that read
		// the bounds with isc_array_lookup_bounds() asks for anyway.
		if (array->dimensions) {

			if (parsedsdl.getDimensionCount()!=
							array->dimensions) {
				return errorResponse("get slice response",
							isc_invalid_sdl);
			}

			int32_t	subscripts[SQLRFIREBIRDSDL_MAX_DIMENSIONS];
			if (!parsedsdl.getSubscripts(i,subscripts) ||
				!parsedsdl.getArrayIndex(subscripts,
							array->lower,
							array->upper,
							&index)) {
				return errorResponse("get slice response",
							isc_invalid_sdl);
			}
		}

		if (index>=array->elementcount) {
			return errorResponse("get slice response",
						isc_invalid_sdl);
		}

		write(&respbuffer,data+index*elementsize,elementsize);
	}

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_slice
	// 	int32_t		slice length
	// 	int32_t		slice length (again)
	// 	byte_t[]	elements
	// }
	//
	// (firebird's P_SLR - see FB4 src/remote/protocol.cpp:616.  The
	// length is written twice because xdr_slice() writes one of its own,
	// and both are the length of the slice in the client's own internal
	// representation, not the length of what goes on the wire - the
	// client divides it by the element width to know how many elements
	// to read.)

	debugStart("get slice response");

	uint32_t	byteswritten=0;

	opcode=op_slice;
	if (!writeInt(opcode,"response op code",&byteswritten)) {
		return false;
	}
	debugOpCode("response op code",opcode);

	uint32_t	responselen=(uint32_t)(count*elementsize);
	if (!writeInt(responselen,"slice length",&byteswritten) ||
		!writeInt(responselen,"slice length",&byteswritten)) {
		return false;
	}

	const byte_t	*elements=respbuffer.getBuffer();
	for (uint64_t i=0; i<count; i++) {
		if (!writeSliceElement(elementtype,elementsize,
					elements+i*elementsize,
					&byteswritten)) {
			return false;
		}
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

bool sqlrprotocol_firebird::putSlice() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		transaction handle
	// 	int32_t		array id, high word
	// 	int32_t		array id, low word
	// 	int32_t		slice length in the client's own
	// 			representation
	// 	int32_t		sdl length
	// 	byte_t[]	sdl
	// 	byte_t[]	sdl padding
	// 	int32_t		parameters length
	// 	byte_t[]	parameters
	// 	int32_t		slice length (again)
	// 	byte_t[]	elements
	// }
	//
	// (firebird's P_SLC, the same one op_get_slice uses - see FB4
	// src/remote/protocol.cpp:590.  The difference is the trailing slice,
	// which carries the elements on a put and is empty on a get.)

	debugStart("put slice");

	const char	*title="put slice response";

	uint32_t	bytesread=0;

	uint32_t	clienttrhandle;
	uint32_t	high;
	uint32_t	low;
	uint32_t	clientslicelen;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread) ||
		!readInt(&high,"array id high word",&bytesread) ||
		!readInt(&low,"array id low word",&bytesread) ||
		!readInt(&clientslicelen,"slice length",&bytesread)) {
		return false;
	}

	uint32_t	sdllen=0;
	byte_t		*sdl=NULL;
	if (!readBuffer(&sdl,&sdllen,"sdl",&bytesread)) {
		return false;
	}

	// the parameters carry values for any expressions in the sdl, and
	// nothing that generates an sdl here puts an expression in one, so
	// this is read and dropped - the same as in getSlice()
	uint32_t	paramslen=0;
	byte_t		*params=NULL;
	if (!readBuffer(&params,&paramslen,"parameters",&bytesread)) {
		delete[] sdl;
		return false;
	}
	delete[] params;

	uint32_t	slicelen=0;
	if (!readInt(&slicelen,"slice length",&bytesread)) {
		delete[] sdl;
		return false;
	}

	// work out what the client is writing
	sqlrfirebirdsdl	parsedsdl;
	if (!parsedsdl.parse(sdl,sdllen)) {
		if (getDebug()) {
			stdoutput.printf("	sdl parse failed: %s\n",
						parsedsdl.getError());
		}
		delete[] sdl;
		// the elements are xdr-encoded per the sdl's element type
		// (a short still costs a full 4-byte slot, a bool or text
		// element carries its own padding, and so on) - with no
		// type to decode by, there's no way to know how many bytes
		// of slice still have to come off the wire, so the
		// connection can't be trusted for another request
		return false;
	}
	delete[] sdl;

	byte_t		elementtype=parsedsdl.getElementType();
	uint32_t	elementsize=arrayElementSize(elementtype,
						parsedsdl.getElementLength());
	if (!elementsize) {
		// an element type this module has no xdr decoding for -
		// same problem as an unparseable sdl, above: no way to know
		// how many wire bytes the slice still owes, so the
		// connection can't be trusted for another request
		return false;
	}

	uint16_t	dimensions=parsedsdl.getDimensionCount();
	uint64_t	count=parsedsdl.getElementCount();
	if (!count || !dimensions ||
		dimensions>(uint16_t)SQLRFIREBIRDSDL_MAX_DIMENSIONS) {
		// the element type is known here, so the slice can still be
		// drained through readSliceElement() before answering
		if (!drainSliceElements(elementtype,elementsize,
					slicelen/elementsize,&bytesread)) {
			return false;
		}
		debugEnd();
		return errorResponse(title,isc_invalid_sdl);
	}

	// The slice length the client wrote says how many bytes it is
	// sending, in its own representation, so it says how many elements
	// to read.  It can't be trusted past what the sdl describes, and it
	// can't be allowed to allocate whatever it likes either.
	uint64_t	sentcount=slicelen/elementsize;
	if (sentcount>count) {
		sentcount=count;
	}
	// count and elementsize are both attacker-controlled (the sdl came
	// off the wire), so this has to check for overflow rather than just
	// multiplying and comparing - count*elementsize can wrap a 64-bit
	// int and slip an undersized buffer past a straightforward "is the
	// product too big" check
	if (count>MAX_ARRAY_BUFFER/elementsize) {
		if (!drainSliceElements(elementtype,elementsize,
					slicelen/elementsize,&bytesread)) {
			return false;
		}
		debugEnd();
		return errorResponse(title,isc_invalid_sdl);
	}

	// read the elements
	// (everything the slice length promised has to come off the socket,
	// used or not, or the connection desynchronizes)
	uint64_t	buffersize=count*elementsize;
	byte_t		*buffer=new byte_t[(size_t)buffersize];
	bytestring::zero(buffer,(size_t)buffersize);
	for (uint64_t i=0; i<sentcount; i++) {
		if (!readSliceElement(elementtype,elementsize,
					buffer+i*elementsize,&bytesread)) {
			delete[] buffer;
			return false;
		}
	}

	// the client may have sent more elements than the array can hold -
	// sentcount above was clamped to count, but the excess still has to
	// come off the wire
	uint64_t	wantcount=slicelen/elementsize;
	if (wantcount>sentcount &&
		!drainSliceElements(elementtype,elementsize,
					wantcount-sentcount,&bytesread)) {
		delete[] buffer;
		return false;
	}

	debugEnd();

	// An id of 0 asks for a new array - which is what a client that is
	// building an array to bind into an insert or update sends.  A
	// nonzero id rewrites an array the session already has.
	sqlrfirebirdarray	*array=getArrayById(high,low);
	if (!array) {
		if (high || low) {
			delete[] buffer;
			// firebird has no code of its own for a bad array id -
			// its own server reports one out of the same blob-id
			// check
			return errorResponse(title,isc_bad_segstr_id);
		}
		trimArrays();
		array=newArray();
	}

	arraybytes=arraybytes-array->data.getSize();
	array->data.clear();
	array->data.append(buffer,(size_t)buffersize);
	arraybytes=arraybytes+buffersize;

	delete[] buffer;

	array->elementtype=elementtype;
	array->elementscale=parsedsdl.getElementScale();
	array->elementsize=elementsize;
	array->elementcount=count;

	// the bounds the sdl carried are the array's own, since what the
	// client wrote is the whole of it as far as the module is concerned
	array->dimensions=dimensions;
	for (uint16_t i=0; i<dimensions; i++) {
		array->lower[i]=parsedsdl.getLowerBound(i);
		array->upper[i]=parsedsdl.getUpperBound(i);
	}

	successStatusVector();

	// a real firebird server answers op_put_slice with the array's id,
	// new or not, in the response's blob id
	return genericResponse(title,
				0,
				(((uint64_t)ARRAY_ID_HIGH)<<32)|array->id,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::cancel() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		kind
	// }

	debugStart("cancel");

	uint32_t	bytesread=0;

	uint32_t	kind;
	if (!readInt(&kind,"kind",&bytesread)) {
		return false;
	}

	debugEnd();

	// answer nothing - a client sends this unsolicited and reads no reply,
	// so answering desynchronizes the connection, and there is nothing to
	// cancel while the module is reading a request anyway
	return true;
}

bool sqlrprotocol_firebird::batchCreate() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	cstring		blr describing the message
	// 	int32_t		message length
	// 	cstring		batch parameter buffer
	// }

	debugStart("batch create");

	const char	*title="batch create response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	sqlrfirebirdfield	*fields=NULL;
	uint16_t		fieldcount=0;
	uint32_t		blrgdscode=0;
	if (!readBlr(&fields,&fieldcount,"message blr",
					&bytesread,&blrgdscode)) {
		// the rest of the request is still on the socket, so the
		// session ends either way - but the client hears why first
		if (blrgdscode) {
			errorResponse(title,blrgdscode);
		}
		return false;
	}

	// the length of one message in the client's buffer, which means
	// nothing here - the wire format is packed, and the blr says what it
	// contains
	uint32_t	msglen;
	if (!readInt(&msglen,"message length",&bytesread)) {
		delete[] fields;
		return false;
	}

	uint32_t	pblen=0;
	byte_t		*pb=NULL;
	if (!readBuffer(&pb,&pblen,"batch param buffer",&bytesread)) {
		delete[] fields;
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt) {
		delete[] fields;
		delete[] pb;
		return errorResponse(title,isc_bad_stmt_handle);
	}

	// a statement can only run one batch at a time
	if (stmt->batch.open) {
		delete[] fields;
		delete[] pb;
		return errorResponse(title,isc_batch_open);
	}

	clearBatch(&stmt->batch);

	uint32_t	pbgdscode=0;
	bool		pbparsed=parseBatchPb(pb,pblen,
						&stmt->batch,&pbgdscode);

	delete[] pb;

	if (!pbparsed) {
		clearBatch(&stmt->batch);
		delete[] fields;
		return errorResponse(title,pbgdscode);
	}

	stmt->batch.fields=fields;
	stmt->batch.fieldcount=fieldcount;
	stmt->batch.open=true;

	successStatusVector();

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::batchMsg() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	int32_t		message count
	// 	message[]	that many packed messages
	// }
	//
	// The messages run back-to-back, each one packed the way
	// readMessage() reads it, rather than being wrapped in a counted
	// buffer of their own - see the op_batch_msg case in firebird's
	// src/remote/protocol.cpp, which decodes them with the same
	// xdr_packed_message() a fetch uses.

	debugStart("batch msg");

	const char	*title="batch msg response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	msgcount;
	if (!readInt(&msgcount,"message count",&bytesread)) {
		return false;
	}

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);

	// without the batch's message format there is no way to tell where
	// the messages end, so the connection can't be resynchronized - a
	// real firebird fails the same way, in its decoder, and drops the
	// connection
	if (!stmt || !stmt->batch.open) {
		errorResponse(title,isc_bad_batch_handle);
		return false;
	}

	sqlrfirebirdbatch	*batch=&stmt->batch;

	for (uint32_t m=0; m<msgcount; m++) {

		byte_t	*nullbits=NULL;
		if (!readMessageNullBits(batch->fieldcount,
						&nullbits,&bytesread)) {
			return false;
		}

		sqlrfirebirdvalue	*values=
				new sqlrfirebirdvalue[batch->fieldcount];
		for (uint16_t i=0; i<batch->fieldcount; i++) {
			values[i].strval=NULL;
		}

		bool	valuesread=true;
		for (uint16_t i=0; i<batch->fieldcount; i++) {
			if (!readMessageValue(&batch->fields[i],
						nullbits,i,true,&values[i],
						&bytesread)) {
				valuesread=false;
				break;
			}
		}

		delete[] nullbits;

		if (!valuesread) {
			for (uint16_t i=0; i<batch->fieldcount; i++) {
				delete[] values[i].strval;
			}
			delete[] values;
			return false;
		}

		sqlrfirebirdbatchmessage	*msg=
					new sqlrfirebirdbatchmessage;
		msg->values=values;
		msg->valuecount=batch->fieldcount;
		batch->messages.append(msg);
	}

	// what the batch has cost so far, against the ceiling the batch
	// parameter buffer set
	batch->queuedbytes=batch->queuedbytes+bytesread;

	if (getDebug()) {
		stdoutput.printf("	queued %u message(s), "
					"%lld byte(s) in all\n",
					msgcount,
					(long long)batch->queuedbytes);
	}

	debugEnd();

	if (batch->queuedbytes>batch->buffersize) {
		clearBatchMessages(batch);
		return errorResponse(title,isc_batch_too_big);
	}

	successStatusVector();

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::batchExec() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	int32_t		transaction handle
	// }

	debugStart("batch exec");

	const char	*title="batch exec response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	clienttrhandle;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->batch.open) {
		return errorResponse(title,isc_bad_batch_handle);
	}
	if (!cursor || !stmt->prepared) {
		return errorResponse(title,isc_bad_stmt_handle);
	}

	// refuse a write in a read-only transaction, the same way an ordinary
	// execute does - a batch is only ever a write
	if (trreadonly) {
		return errorResponse(title,isc_read_only_trans);
	}

	sqlrfirebirdbatch	*batch=&stmt->batch;

	uint32_t	msgcount=(uint32_t)batch->messages.getCount();

	// per-message row counts, kept only when the batch asked for them
	uint32_t	*updates=NULL;
	if (batch->recordcounts && msgcount) {
		updates=new uint32_t[msgcount];
	}

	linkedlist< sqlrfirebirdbatcherror * >	errors;

	// run the queued messages, in the order they were queued
	// (there's no array bind in the server api, so a batch is a loop of
	// ordinary executes over the statement the client already prepared)
	uint32_t	position=0;
	for (listnode< sqlrfirebirdbatchmessage * > *node=
					batch->messages.getFirst();
					node; node=node->getNext()) {

		uint32_t	affected=0;
		bool		ran=execBatchMessage(cursor,batch,
						node->getValue(),
						position,&affected,&errors);

		if (updates) {
			updates[position]=affected;
		}

		position++;

		// without multierror the first failure ends the run, and the
		// messages behind it are never tried
		if (!ran && !batch->multierror) {
			break;
		}
	}

	if (getDebug()) {
		stdoutput.printf("	ran %u message(s), %u failed\n",
					position,
					(uint32_t)errors.getCount());
	}

	// execute consumes the queue, whether or not everything in it ran -
	// the batch itself stays open, and the client can queue more
	clearBatchMessages(batch);

	bool	sent=batchCompletionState(title,stmthandle,batch,
						position,updates,&errors);

	delete[] updates;
	for (listnode< sqlrfirebirdbatcherror * > *node=errors.getFirst();
						node; node=node->getNext()) {
		delete[] node->getValue()->message;
		delete node->getValue();
	}

	return sent;
}

bool sqlrprotocol_firebird::execBatchMessage(sqlrservercursor *cursor,
					sqlrfirebirdbatch *batch,
					sqlrfirebirdbatchmessage *msg,
					uint32_t position,
					uint32_t *affected,
					linkedlist< sqlrfirebirdbatcherror * >
								*errors) {

	*affected=0;

	// the binds are about to be refilled - see executeStatement()
	memorypool		*bindpool=cont->getBindPool(cursor);
	sqlrserverbindvar	*inbinds=cont->getInputBinds(cursor);
	bindpool->clear();

	badblobid=false;

	uint16_t	bindcount=0;
	for (uint16_t i=0; i<msg->valuecount && bindcount<maxbindcount; i++) {

		// a message names a blob by the id the client gave the batch,
		// which has to be translated to the module's own id before
		// the bind can find the bytes
		sqlrfirebirdvalue	val=msg->values[i];
		if (val.isblob) {
			sqlrfirebirdbatchblob	*bb=getBatchBlob(batch,
							val.blobhigh,
							val.bloblow);
			if (bb) {
				val.blobhigh=0;
				val.bloblow=bb->blobid;
			}
		}

		bindMessageValue(bindpool,&(inbinds[bindcount]),bindcount,
						&batch->fields[i],&val);
		bindcount++;
	}

	cont->setInputBindCount(cursor,bindcount);

	if (getDebug()) {
		stdoutput.printf("	message %u: bound %u parameter(s)\n",
							position,bindcount);
	}

	sqlrfirebirdbatcherror	*err=NULL;

	if (badblobid) {

		err=new sqlrfirebirdbatcherror;
		err->position=position;
		err->gdscode=isc_bad_segstr_id;
		err->sqlcode=-901;
		err->message=NULL;

	} else if (!cont->executeQuery(cursor,true,true,true,true)) {

		const char	*errorstring=NULL;
		uint32_t	errorsize=0;
		int64_t		errnum=0;
		bool		liveconnection=true;
		cont->getError(cursor,&errorstring,&errorsize,
					&errnum,&liveconnection);

		// see sendCursorError() for what makes a sql code out of
		// whatever number the backend reported
		int32_t	sqlcode=(int32_t)errnum;
		if (sqlcode>=0) {
			sqlcode=-901;
		}

		err=new sqlrfirebirdbatcherror;
		err->position=position;
		err->gdscode=isc_random;
		err->sqlcode=sqlcode;
		err->message=charstring::duplicate(errorstring,errorsize);
	}

	if (err) {
		errors->append(err);
		return false;
	}

	if (cont->knowsAffectedRows(cursor)) {
		*affected=(uint32_t)cont->getAffectedRows(cursor);
	}

	return true;
}

bool sqlrprotocol_firebird::batchCompletionState(const char *title,
					uint32_t stmthandle,
					sqlrfirebirdbatch *batch,
					uint32_t reccount,
					uint32_t *updates,
					linkedlist< sqlrfirebirdbatcherror * >
								*errors) {

	// response packet data structure:
	//
	// data {
	// 	int32_t		op_batch_cs
	// 	int32_t		statement handle
	// 	int32_t		message count
	// 	int32_t		update count count
	// 	int32_t		detailed failure count
	// 	int32_t		plain failure count
	// 	int32_t[]	one update count per message
	// 	{int32_t,int32_t[]}[]	position, status vector
	// 	int32_t[]	the position of each plain failure
	// }
	//
	// (a batch answers this instead of an op_response - the errors it
	// reports belong to individual messages, and a status vector of the
	// whole request has nowhere to put them.  the two failure counts
	// name disjoint sets - a failure is either detailed by a status
	// vector or named by position alone, never both)

	debugStart(title);

	uint32_t	byteswritten=0;

	uint32_t	updatecount=(updates)?reccount:0;
	uint32_t	errorcount=(uint32_t)errors->getCount();

	// only as many failures are detailed as the batch parameter buffer
	// asked to hear about - the rest are named by position alone
	uint32_t	vectorcount=(errorcount<batch->detailederrors)?
					errorcount:batch->detailederrors;
	uint32_t	plaincount=errorcount-vectorcount;

	opcode=op_batch_cs;
	if (!writeInt(opcode,"response op code",&byteswritten)) {
		return false;
	}
	debugOpCode("response op code",opcode);

	if (!writeInt(stmthandle,"statement handle",&byteswritten) ||
		!writeInt(reccount,"message count",&byteswritten) ||
		!writeInt(updatecount,"update count count",&byteswritten) ||
		!writeInt(vectorcount,
				"detailed failure count",&byteswritten) ||
		!writeInt(plaincount,
				"plain failure count",&byteswritten)) {
		return false;
	}

	// the update counts
	for (uint32_t i=0; i<updatecount; i++) {
		if (!writeInt(updates[i],"update count",&byteswritten)) {
			return false;
		}
	}

	// a status vector for each failure that gets detailed
	uint32_t	detailed=0;
	for (listnode< sqlrfirebirdbatcherror * > *node=errors->getFirst();
				node && detailed<vectorcount;
				node=node->getNext()) {

		sqlrfirebirdbatcherror	*err=node->getValue();

		if (!writeInt(err->position,
				"status vector position",&byteswritten)) {
			return false;
		}

		// what errorResponse() builds, built here instead because
		// several of them go out in one response
		uint32_t	sv[12];
		const char	*svstr[12];
		bytestring::zero(sv,sizeof(sv));
		bytestring::zero(svstr,sizeof(svstr));

		uint8_t	i=0;
		sv[i++]=isc_arg_gds;
		sv[i++]=err->gdscode;
		if (err->message) {
			sv[i++]=isc_arg_string;
			svstr[i++]=err->message;
		}
		sv[i++]=isc_arg_sql_state;
		svstr[i++]=sqlStateForSqlCode(err->sqlcode);
		sv[i++]=isc_arg_gds;
		sv[i++]=isc_sqlerr;
		sv[i++]=isc_arg_number;
		sv[i++]=(uint32_t)err->sqlcode;
		sv[i++]=isc_arg_end;

		if (!writeStatusVector(sv,svstr,i,&byteswritten)) {
			return false;
		}

		detailed++;
	}

	// the position of each failure that didn't get one, skipping the ones
	// that did
	uint32_t	skipped=0;
	for (listnode< sqlrfirebirdbatcherror * > *node=errors->getFirst();
						node; node=node->getNext()) {
		if (skipped<vectorcount) {
			skipped++;
			continue;
		}
		if (!writeInt(node->getValue()->position,
					"error position",&byteswritten)) {
			return false;
		}
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

bool sqlrprotocol_firebird::batchRls() {
	return batchRelease("batch rls response",false);
}

// a batch has nothing in flight of its own to abort - the messages it holds
// haven't run, and the ones a previous op_batch_exec did run are part of the
// transaction, which only a commit or rollback can undo.  So cancelling and
// releasing come to the same thing here, and only the op the client sent
// tells them apart.
bool sqlrprotocol_firebird::batchCancel() {
	return batchRelease("batch cancel response",true);
}

bool sqlrprotocol_firebird::batchRelease(const char *title, bool cancel) {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// }

	debugStart((cancel)?"batch cancel":"batch rls");

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->batch.open) {
		return errorResponse(title,isc_bad_batch_handle);
	}

	// only the batch goes - the statement stays prepared, and the client
	// can keep executing it without a batch
	clearBatch(&stmt->batch);

	successStatusVector();

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::batchSync() {

	// request packet data structure:
	//
	// data {
	// }
	//
	// The op carries nothing at all, not even a statement handle - it
	// asks the session to answer everything it still owes, and the
	// answer is an ordinary response.  Nothing here defers a response,
	// so there is never anything outstanding to flush.

	debugStart("batch sync");
	debugEnd();

	successStatusVector();

	return genericResponse("batch sync response",
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::batchSetBpb() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	cstring		blob parameter buffer
	// }
	//
	// The buffer is a plain bpb, not a batch parameter buffer - it sets
	// the default the batch gives blobs the client registers with it.

	debugStart("batch set bpb");

	const char	*title="batch set bpb response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	bpblen=0;
	byte_t		*bpb=NULL;
	if (!readBuffer(&bpb,&bpblen,"blob param buffer",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->batch.open) {
		delete[] bpb;
		return errorResponse(title,isc_bad_batch_handle);
	}

	// parseBpb() only ever sets isstream, and a blob is segmented unless
	// the bpb says otherwise
	sqlrfirebirdblob	tmpblob;
	tmpblob.isstream=false;
	parseBpb(bpb,bpblen,&tmpblob);
	stmt->batch.blobsegmented=!tmpblob.isstream;

	delete[] bpb;

	successStatusVector();

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::batchRegBlob() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	int32_t		existing blob id, high word
	// 	int32_t		existing blob id, low word
	// 	int32_t		batch blob id, high word
	// 	int32_t		batch blob id, low word
	// }
	//
	// The client is saying "inside this batch, the blob I already built
	// is called this" - the messages it queues refer to the blob by the
	// second id, which it made up itself, and which means nothing outside
	// the batch.

	debugStart("batch regblob");

	const char	*title="batch regblob response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	uint32_t	existhigh;
	uint32_t	existlow;
	if (!readInt(&existhigh,"existing blob id high word",&bytesread) ||
		!readInt(&existlow,"existing blob id low word",&bytesread)) {
		return false;
	}

	uint32_t	temphigh;
	uint32_t	templow;
	if (!readInt(&temphigh,"batch blob id high word",&bytesread) ||
		!readInt(&templow,"batch blob id low word",&bytesread)) {
		return false;
	}

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->batch.open) {
		return errorResponse(title,isc_bad_batch_handle);
	}

	sqlrfirebirdblob	*blob=getBlobById(existhigh,existlow);
	if (!blob) {
		return errorResponse(title,isc_bad_segstr_id);
	}

	setBatchBlob(&stmt->batch,temphigh,templow,blob->id);

	successStatusVector();

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::batchBlobStream() {

	// request packet data structure:
	//
	// data {
	// 	int32_t		statement handle
	// 	cstring		blob stream
	// }

	debugStart("batch blob stream");

	const char	*title="batch blob stream response";

	uint32_t	bytesread=0;

	uint32_t	stmthandle;
	if (!readInt(&stmthandle,"statement handle",&bytesread)) {
		return false;
	}

	// the stream isn't read as a buffer, because the length that follows
	// isn't the number of bytes that follow it - see
	// parseBatchBlobStream()
	uint32_t	streamlen=0;
	if (!readInt(&streamlen,"blob stream length",&bytesread)) {
		return false;
	}

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->batch.open) {
		// the stream can't be decoded, let alone skipped, without the
		// batch's state, so there's no way to find where the next op
		// starts and carry on.  a real firebird fails the packet here
		// too, which drops the connection.
		if (getDebug()) {
			stdoutput.write("	no such batch\n");
			debugEnd();
		}
		return false;
	}

	// same again - a stream that didn't decode leaves the session part
	// way through an op, with nothing to resynchronize on
	if (!parseBatchBlobStream(&stmt->batch,streamlen,&bytesread)) {
		if (getDebug()) {
			stdoutput.write("	invalid blob stream\n");
			debugEnd();
		}
		return false;
	}

	debugEnd();

	successStatusVector();

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::parseBatchBlobStream(sqlrfirebirdbatch *batch,
						uint32_t streamlen,
						uint32_t *bytesread) {

	// A stream is one blob after another, each of them a header - an
	// 8-byte batch blob id, a 4-byte length and a 4-byte blob parameter
	// buffer length, all of them big-endian, with the id's high word
	// first - then that many parameter buffer bytes, then the blob's own
	// bytes.  A stream blob's bytes are one run, a segmented blob's are
	// segments, each of them a length and that many bytes.  The length in
	// the header covers the parameter buffer and the segment lengths as
	// well as the bytes.
	//
	// The length the op carries isn't the number of bytes that follow it.
	// It's the length of the buffer the client laid the stream out in,
	// and the wire only carries the parts of that buffer that mean
	// something:
	//
	// - the bytes that pad a blob header or a segment length back into
	//   alignment are in the buffer, but never sent
	// - a segment length takes 2 bytes of the buffer, but 4 on the wire,
	//   because xdr sends even a 16-bit value as a 4-byte big-endian one
	// - a header that would run off the end of the buffer isn't sent at
	//   all.  The client holds it back and sends it whole at the front of
	//   the next op, so the buffer's last few bytes just go missing.
	//
	// So the stream has to be decoded as it's read, counting buffer bytes
	// and bytes off the wire separately, and nothing here can read ahead.
	// See xdr_blob_stream() in firebird's src/remote/protocol.cpp.
	//
	// A blob, a parameter buffer or a segment can also break where the
	// buffer ends and carry on in the next op, which is why how far the
	// last one got is kept on the batch rather than here.

	// what the client's buffer holds is a whole number of alignment units
	if (streamlen%BATCH_blob_stream_align) {
		return false;
	}

	// how much of the client's buffer is left, and how far into it we
	// are, which is what alignment is measured against, since the buffer
	// itself starts out aligned
	uint32_t	remains=streamlen;
	uint32_t	offset=0;

	while (remains) {

		// the next blob's header
		if (!batch->blobstreamremaining) {

			// align
			uint32_t	pad=offset%BATCH_blob_stream_align;
			if (pad) {
				pad=BATCH_blob_stream_align-pad;
				if (pad>remains) {
					return false;
				}
				remains=remains-pad;
				offset=offset+pad;
				continue;
			}

			// a header that wouldn't fit is one the client held
			// back - what's left of the buffer was never sent
			if (remains<BATCH_blob_hdr_size) {
				break;
			}

			uint32_t	temphigh=0;
			uint32_t	templow=0;
			uint32_t	bloblen=0;
			uint32_t	bpblen=0;
			if (!readInt(&temphigh,"batch blob id high word",
								bytesread) ||
				!readInt(&templow,"batch blob id low word",
								bytesread) ||
				!readInt(&bloblen,"batch blob length",
								bytesread) ||
				!readInt(&bpblen,"batch blob bpb length",
								bytesread)) {
				return false;
			}
			remains=remains-BATCH_blob_hdr_size;
			offset=offset+BATCH_blob_hdr_size;

			if (bpblen>bloblen) {
				return false;
			}

			// the blob the batch's messages will bind
			sqlrfirebirdblob	*blob=newBlob();
			blob->iswrite=true;
			setBatchBlob(batch,temphigh,templow,blob->id);

			batch->blobstreamblobid=blob->id;
			batch->blobstreamremaining=bloblen;
			batch->blobstreambpbremaining=bpblen;
			batch->blobstreamsegremaining=0;
			batch->blobstreambpb.clear();
			batch->blobstreamdata.clear();

			// without a parameter buffer of its own, the blob is
			// whatever op_batch_set_bpb left behind
			if (!bpblen) {
				batch->blobstreamsegmented=
						batch->blobsegmented;
			}
			continue;
		}

		// the blob the header started, which the ops before this one
		// may have started instead
		sqlrfirebirdblob	*blob=
				getBlobById(0,batch->blobstreamblobid);
		if (!blob) {
			return false;
		}

		// the blob's parameter buffer
		if (batch->blobstreambpbremaining) {

			uint32_t	size=
				(batch->blobstreambpbremaining<remains)?
					batch->blobstreambpbremaining:remains;
			if (!readBytes(&batch->blobstreambpb,size,
						"batch blob bpb",bytesread)) {
				return false;
			}
			batch->blobstreambpbremaining=
				batch->blobstreambpbremaining-size;
			batch->blobstreamremaining=
				batch->blobstreamremaining-size;
			remains=remains-size;
			offset=offset+size;

			// the blob's own parameter buffer wins over the one
			// op_batch_set_bpb left behind, for this blob only
			if (!batch->blobstreambpbremaining) {
				sqlrfirebirdblob	tmpblob;
				tmpblob.isstream=false;
				parseBpb(batch->blobstreambpb.getBuffer(),
					(uint32_t)batch->
						blobstreambpb.getSize(),
					&tmpblob);
				batch->blobstreamsegmented=!tmpblob.isstream;
				batch->blobstreambpb.clear();
			}
			continue;
		}

		if (batch->blobstreamsegmented) {

			// the next segment's length
			if (!batch->blobstreamsegremaining) {

				// align
				uint32_t	pad=
					offset%BATCH_blob_seghdr_align;
				if (pad) {
					pad=BATCH_blob_seghdr_align-pad;
					if (pad>remains ||
						pad>batch->
						blobstreamremaining) {
						return false;
					}
					remains=remains-pad;
					offset=offset+pad;
					batch->blobstreamremaining=
						batch->blobstreamremaining-
									pad;
					continue;
				}

				if (BATCH_blob_seghdr_size>remains ||
					BATCH_blob_seghdr_size>
						batch->blobstreamremaining) {
					return false;
				}

				// only the low 16 bits are the length - the
				// other 2 bytes are the padding that xdr
				// sends a 16-bit value in
				uint32_t	seglen=0;
				if (!readInt(&seglen,
						"batch blob segment length",
						bytesread)) {
					return false;
				}
				seglen=seglen&0xffff;

				remains=remains-BATCH_blob_seghdr_size;
				offset=offset+BATCH_blob_seghdr_size;
				batch->blobstreamremaining=
					batch->blobstreamremaining-
						BATCH_blob_seghdr_size;

				if (seglen>batch->blobstreamremaining) {
					return false;
				}
				batch->blobstreamsegremaining=seglen;
				continue;
			}

			// the segment's bytes
			uint32_t	size=
				(batch->blobstreamsegremaining<remains)?
					batch->blobstreamsegremaining:remains;
			if (!readBytes(&batch->blobstreamdata,size,
					"batch blob segment",bytesread)) {
				return false;
			}
			batch->blobstreamsegremaining=
				batch->blobstreamsegremaining-size;
			batch->blobstreamremaining=
				batch->blobstreamremaining-size;
			remains=remains-size;
			offset=offset+size;

			if (!batch->blobstreamsegremaining) {
				appendBlobSegment(blob,
					batch->blobstreamdata.getBuffer(),
					(uint32_t)batch->
						blobstreamdata.getSize());
				batch->blobstreamdata.clear();
			}
			continue;
		}

		// a stream blob's bytes, which the module holds as a single
		// segment, so they're gathered up until the blob ends
		uint32_t	size=(batch->blobstreamremaining<remains)?
					batch->blobstreamremaining:remains;
		if (!readBytes(&batch->blobstreamdata,size,
					"batch blob data",bytesread)) {
			return false;
		}
		batch->blobstreamremaining=batch->blobstreamremaining-size;
		remains=remains-size;
		offset=offset+size;

		if (!batch->blobstreamremaining) {
			appendBlobSegment(blob,
				batch->blobstreamdata.getBuffer(),
				(uint32_t)batch->blobstreamdata.getSize());
			batch->blobstreamdata.clear();
		}
	}

	trimBlobs();

	return true;
}

// nothing backs the service manager - no backup, restore, repair, or
// statistics entry point on any backend - so answering these ops for real
// would let a client believe an operation completed when nothing happened
// behind it. Refuse, but with isc_service_att_err rather than the generic
// isc_wish_list, so the error names the service manager instead of just
// "feature is not supported".
bool sqlrprotocol_firebird::serviceAttach() {
	return sendServiceAttachError();
}

bool sqlrprotocol_firebird::serviceDetach() {
	return sendServiceAttachError();
}

bool sqlrprotocol_firebird::serviceStart() {
	return sendServiceAttachError();
}

bool sqlrprotocol_firebird::serviceInfo() {
	return sendServiceAttachError();
}

// connectRequest(), queEvents() and cancelEvents() are firebird's
// asynchronous event notification: a trigger or procedure posts an event,
// and a client that queued one gets told. Answering op_connect_request
// would mean opening a second, server-owned socket and accepting the
// client's connection to it alongside the live session - a generalization
// of suspendSession() (see sqlrclient.cpp), not a call to it. But nothing
// generates an event to deliver even if that channel existed: the firebird
// connection module never registers for one (no isc_wait_for_event /
// isc_que_events), and the server api has no registration, callback, or
// path of its own to carry a database event to a protocol module -
// sqlrevent_t is unrelated, it's server-side logging and alerting, not a
// database event a backend posts. Refusing here is correct until that
// plumbing exists.
bool sqlrprotocol_firebird::connectRequest() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::queEvents() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::cancelEvents() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::sendNotImplementedError() {

	// the unread request is still on the socket, so the connection can't
	// be reused - the session ends either way, but it ends after the
	// client has been told what went wrong
	errorResponse("not implemented response",
			isc_wish_list,"0A000",-901,
			"Feature is not supported",24);
	return false;
}

bool sqlrprotocol_firebird::sendServiceAttachError() {
	errorResponse("service manager not implemented",
			isc_service_att_err,"HY000",-904,
			"Cannot attach to services manager",33);
	return false;
}

bool sqlrprotocol_firebird::errorResponse(const char *title,
						uint32_t gdscode) {
	bytestring::zero(statusvector,sizeof(statusvector));
	bytestring::zero(statusvectorstr,sizeof(statusvectorstr));
	statusvector[0]=isc_arg_gds;
	statusvector[1]=gdscode;
	statusvector[2]=isc_arg_end;
	statusvectorlen=3;
	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::errorResponse(const char *title,
						uint32_t gdscode,
						const char *sqlstate,
						int32_t sqlcode,
						const char *message,
						uint32_t messagesize) {

	// the buffers the vector points into have to outlive this call, since
	// genericResponse() writes from them
	// (the message is copied by its length rather than as a string - the
	// server API hands back a buffer with a size, and reading it as a
	// string runs off the end of the text)
	errormessage.clear();
	if (message && messagesize) {
		errormessage.append(message,messagesize);
	} else {
		errormessage.append("error");
	}
	charstring::copy(errorsqlstate,
			(charstring::isNullOrEmpty(sqlstate))?"42000":sqlstate,
			sizeof(errorsqlstate)-1);
	errorsqlstate[sizeof(errorsqlstate)-1]='\0';

	bytestring::zero(statusvector,sizeof(statusvector));
	bytestring::zero(statusvectorstr,sizeof(statusvectorstr));

	// the leading code is what the client renders as the first message and
	// what it looks the sql state up under
	// (isc_arg_sql_state overrides the lookup, and isc_sqlerr with
	// isc_arg_number overrides the sql code, so the backend's own survives)
	uint8_t	i=0;
	statusvector[i++]=isc_arg_gds;
	statusvector[i++]=gdscode;

	// isc_random's whole message template is "@1", so a message written as
	// its argument becomes the message the client renders.  Every other
	// leading code has a template of its own, and the backend's text
	// trails it, already rendered - which is what isc_arg_interpreted
	// means, as opposed to isc_arg_string.
	if (gdscode==isc_random) {
		statusvector[i++]=isc_arg_string;
		statusvectorstr[i++]=errormessage.getString();
	}

	statusvector[i++]=isc_arg_sql_state;
	statusvectorstr[i++]=errorsqlstate;
	statusvector[i++]=isc_arg_gds;
	statusvector[i++]=isc_sqlerr;
	statusvector[i++]=isc_arg_number;
	statusvector[i++]=(uint32_t)sqlcode;

	if (gdscode!=isc_random) {
		statusvector[i++]=isc_arg_interpreted;
		statusvectorstr[i++]=errormessage.getString();
	}

	statusvector[i++]=isc_arg_end;
	statusvectorlen=i;

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::sendCursorError(const char *title,
						sqlrservercursor *cursor,
						bool preparing) {

	const char	*errorstring=NULL;
	uint32_t	errorsize=0;
	int64_t		errnum=0;
	bool		liveconnection=true;
	if (cursor) {
		cont->getError(cursor,&errorstring,&errorsize,
					&errnum,&liveconnection);
	} else {
		cont->getError(&errorstring,&errorsize,
					&errnum,&liveconnection);
	}

	// the firebird backend reports the sql code, negative, which is what a
	// firebird client expects to read out of the vector - any other
	// backend reports whatever it reports, and a positive number isn't a
	// sql code at all
	int32_t	sqlcode=(int32_t)errnum;
	if (sqlcode>=0) {
		sqlcode=-901;
	}

	// a real firebird leads a prepare error with isc_dsql_error, which
	// renders as "Dynamic SQL Error", and a runtime error with whatever
	// the failure was
	// (the module can't reach the individual runtime codes, so the second
	// case leads with isc_random and lets the backend's text stand)
	return errorResponse(title,
				(preparing)?isc_dsql_error:isc_random,
				sqlStateForSqlCode(sqlcode),
				sqlcode,errorstring,errorsize);
}

const char *sqlrprotocol_firebird::sqlStateForSqlCode(int32_t sqlcode) {

	// what firebird's own gds-code-to-sql-state table answers for the
	// gds code each of these sql codes comes from
	switch (sqlcode) {
		case -104:
			return "42000";
		case -204:
			return "42S02";
		case -206:
			return "42S22";
		case -401:
		case -402:
		case -413:
			return "22018";
		case -501:
		case -502:
		case -504:
			return "24000";
		case -530:
		case -531:
		case -532:
			return "23000";
		case -551:
		case -552:
			return "28000";
		case -625:
		case -803:
			return "23000";
		case -802:
			return "22003";
		case -804:
			return "07002";
		case -901:
			return "HY000";
		case -913:
			return "40001";
		default:
			return "42000";
	}
}

void sqlrprotocol_firebird::describeBinds(sqlrservercursor *cursor,
					sqlrfirebirdstatement *stmt,
					const byte_t *items,
					uint32_t itemslen) {

	// a firebird client expects isc_dsql_describe_bind to answer with
	// each parameter's type, which the module works out for itself
	// FIXME: ask the backend first, with getInputBindCountFromPrepare()
	// and getInputBindType() - only the firebird backend implements them
	// (for an insert the type of a value going into a column is the
	// column's type, so a select of the columns the binds feed answers it)

	// the probe below costs a query, so it only runs when the client
	// actually asked for the bind describe, and only once per prepare
	if (stmt->bindsdescribed) {
		return;
	}
	bool	asked=false;
	for (uint32_t i=0; i<itemslen && !asked; i++) {
		asked=(items[i]==isc_info_sql_bind);
	}
	if (!asked) {
		return;
	}
	stmt->bindsdescribed=true;

	// build the probe query
	stringbuffer	probe;
	uint16_t	bindcount=0;
	if (!buildBindProbe(cursor,&probe,&bindcount)) {
		return;
	}

	const char	*probequery=probe.getString();
	uint32_t	probequerylen=(uint32_t)probe.getSize();

	if (getDebug()) {
		stdoutput.printf("	bind describe probe: \"%s\"\n",
							probequery);
	}

	// the probe is built from the table's own column names, not from the
	// client's query, so a wide table can make it longer than the query
	// that asked for it (the cursor's buffer is maxquerysize+1)
	if (probequerylen>maxquerysize) {
		if (getDebug()) {
			stdoutput.printf("	bind describe probe too "
						"large - got %u, max %u\n",
						probequerylen,maxquerysize);
		}
		return;
	}

	// run it on a cursor of its own
	sqlrservercursor	*probecursor=cont->getCursor();
	if (!probecursor) {
		return;
	}
	cont->setRequireBindVariableTranslation(probecursor,true);

	// a pooled cursor comes back with whatever binds its last user left
	// on it, and nothing between release() and prepare clears them
	cont->getBindPool(probecursor)->clear();
	cont->setInputBindCount(probecursor,0);

	char	*querybuffer=cont->getQueryBuffer(probecursor);
	bytestring::copy(querybuffer,probequery,probequerylen);
	querybuffer[probequerylen]='\0';
	cont->setQuerySize(probecursor,probequerylen);

	if (cont->prepareQuery(probecursor,querybuffer,probequerylen,
					true,true,true,true) &&
		cont->executeQuery(probecursor,true,true,true,true) &&
		cont->colCount(probecursor)==bindcount) {

		stmt->binds=new sqlrfirebirdbind[bindcount];
		stmt->bindcount=bindcount;
		for (uint16_t i=0; i<bindcount; i++) {
			stmt->binds[i].coltype=
				cont->getColumnType(probecursor,i);
			stmt->binds[i].colsize=
				cont->getColumnSize(probecursor,i);
			stmt->binds[i].colscale=
				cont->getColumnScale(probecursor,i);
		}

	} else if (getDebug()) {
		stdoutput.printf("	bind describe probe failed\n");
	}

	cont->closeResultSet(probecursor);
	cont->release(probecursor);
}

bool sqlrprotocol_firebird::buildBindProbe(sqlrservercursor *cursor,
						stringbuffer *probe,
						uint16_t *bindcount) {

	const char	*query=cont->getQueryBuffer(cursor);
	uint32_t	querylen=cont->getQuerySize(cursor);

	*bindcount=cont->countBindVariables(query,querylen);
	if (!*bindcount) {
		return false;
	}

	sqlrquerytype_t		querytype=SQLRQUERYTYPE_ETC;
	char			*table=NULL;
	linkedlist<char *>	*columns=NULL;
	linkedlist<char *>	*values=NULL;
	bool	parsed=cont->parseInsert(query,querylen,&querytype,&table,
					&columns,NULL,NULL,NULL,NULL,NULL,
					&values,NULL);

	// a multi-insert or insert-select describes no binds of its own
	bool	usable=parsed && querytype==SQLRQUERYTYPE_INSERT &&
			table && columns && values &&
			columns->getCount()==values->getCount();

	// pair each value that is a bind marker with its column
	uint16_t	found=0;
	if (usable) {
		probe->append("select ");
		listnode<char *>	*cnode=columns->getFirst();
		listnode<char *>	*vnode=values->getFirst();
		for (; cnode && vnode;
			cnode=cnode->getNext(), vnode=vnode->getNext()) {

			if (!isBindMarker(vnode->getValue())) {
				continue;
			}
			if (found) {
				probe->append(',');
			}
			probe->append(cnode->getValue());
			found++;
		}
		probe->append(" from ")->append(table);
		probe->append(" where 1=0");
	}

	delete[] table;
	delete columns;
	delete values;

	// a bind that isn't a whole value - "values (?+1)" - leaves the count
	// short, and there is no way to tell which column it belonged to, so
	// the whole statement falls back
	return usable && found==*bindcount;
}

bool sqlrprotocol_firebird::isBindMarker(const char *value) {

	if (!value) {
		return false;
	}

	// parseInsert() doesn't trim the values it splits out
	const char	*p=value;
	while (character::isWhitespace(*p)) {
		p++;
	}

	// take any of the four markers SQL Relay knows - a protocol module
	// can't see the flags countBindVariables() consults, and no literal,
	// identifier or function call in a value list starts with one
	if (*p!='?' && *p!=':' && *p!='@' && *p!='$') {
		return false;
	}
	p++;

	// the marker has to be the whole value - a bind inside an expression,
	// "values (?+1)", feeds no column on its own
	while (character::isAlphanumeric(*p) || *p=='_') {
		p++;
	}
	while (character::isWhitespace(*p)) {
		p++;
	}
	return !*p;
}

bool sqlrprotocol_firebird::describeOutputColumns(sqlrservercursor *cursor,
						sqlrfirebirdstatement *stmt) {

	// build the probe query
	const char	*query=cont->getQueryBuffer(cursor);
	uint32_t	querylen=cont->getQuerySize(cursor);

	stringbuffer	probe;
	if (!cont->substituteNullForBindVariables(query,querylen,&probe)) {
		return false;
	}

	const char	*probequery=probe.getString();
	uint32_t	probequerylen=(uint32_t)probe.getSize();

	if (getDebug()) {
		stdoutput.printf("	column describe probe: \"%s\"\n",
							probequery);
	}

	// NULL is 4 bytes where the marker it replaced was 1, so a query that
	// just fit can come out of the substitution too large for the cursor's
	// buffer (which is maxquerysize+1)
	if (probequerylen>maxquerysize) {
		if (getDebug()) {
			stdoutput.printf("	column describe probe too "
						"large - got %u, max %u\n",
						probequerylen,maxquerysize);
		}
		return false;
	}

	// run it on a cursor of its own, so the client's own cursor is left
	// for the real execute
	sqlrservercursor	*probecursor=cont->getCursor();
	if (!probecursor) {
		return false;
	}
	cont->setRequireBindVariableTranslation(probecursor,true);

	// a pooled cursor comes back with whatever binds its last user left
	// on it, and nothing between release() and prepare clears them
	cont->getBindPool(probecursor)->clear();
	cont->setInputBindCount(probecursor,0);

	char	*querybuffer=cont->getQueryBuffer(probecursor);
	bytestring::copy(querybuffer,probequery,probequerylen);
	querybuffer[probequerylen]='\0';
	cont->setQuerySize(probecursor,probequerylen);

	// the probe has to execute, not just prepare - its own prepare is
	// deferred for the same reason the client's was
	bool	success=false;
	if (cont->prepareQuery(probecursor,querybuffer,probequerylen,
					true,true,true,true) &&
		cont->executeQuery(probecursor,true,true,true,true) &&
		cont->colCount(probecursor)) {

		uint32_t	colcount=cont->colCount(probecursor);

		stmt->probecols=new sqlrfirebirdprobecolumn[colcount];
		stmt->probecolcount=colcount;
		for (uint32_t i=0; i<colcount; i++) {
			stmt->probecols[i].name=charstring::duplicate(
					cont->getColumnName(probecursor,i));
			stmt->probecols[i].table=charstring::duplicate(
					cont->getColumnTable(probecursor,i));
			stmt->probecols[i].coltype=
					cont->getColumnType(probecursor,i);
			stmt->probecols[i].colsize=
					cont->getColumnSize(probecursor,i);
			stmt->probecols[i].colscale=
					cont->getColumnScale(probecursor,i);
		}
		success=true;

	} else if (getDebug()) {
		stdoutput.printf("	column describe probe failed\n");
	}

	cont->closeResultSet(probecursor);
	cont->release(probecursor);

	return success;
}

void sqlrprotocol_firebird::clearProbeColumns(sqlrfirebirdstatement *stmt) {
	for (uint32_t i=0; i<stmt->probecolcount; i++) {
		delete[] stmt->probecols[i].name;
		delete[] stmt->probecols[i].table;
	}
	delete[] stmt->probecols;
	stmt->probecols=NULL;
	stmt->probecolcount=0;
}

sqlrfirebirdstatement *sqlrprotocol_firebird::getStatement(
						uint32_t stmthandle,
						sqlrservercursor **cursor) {

	*cursor=NULL;

	// the handle is the cursor id plus one - see allocateStatement()
	if (!stmthandle || stmthandle>maxcursorcount) {
		return NULL;
	}

	uint16_t	cursorid=(uint16_t)(stmthandle-1);

	*cursor=cont->getCursor(cursorid);
	if (!*cursor) {
		return NULL;
	}

	// the wire format only ever binds by "?", so a query has to be
	// translated to whatever format the backend actually requires
	cont->setRequireBindVariableTranslation(*cursor,true);

	return &statements[cursorid];
}

void sqlrprotocol_firebird::clearStatement(uint16_t cursorid) {
	sqlrfirebirdstatement	*stmt=&statements[cursorid];
	stmt->stmttype=0;
	stmt->prepared=false;
	stmt->preexecuted=false;
	stmt->cursoropen=false;
	delete[] stmt->cursorname;
	stmt->cursorname=NULL;
	delete[] stmt->outfields;
	stmt->outfields=NULL;
	stmt->outfieldcount=0;
	delete[] stmt->binds;
	stmt->binds=NULL;
	stmt->bindcount=0;
	stmt->bindsdescribed=false;
	clearProbeColumns(stmt);
	clearBatch(&stmt->batch);
	clearBlrRequest(&stmt->request);
}

void sqlrprotocol_firebird::clearStatements() {
	for (uint16_t i=0; i<maxcursorcount; i++) {
		clearStatement(i);
	}
}

void sqlrprotocol_firebird::initBatch(sqlrfirebirdbatch *batch) {
	batch->open=false;
	batch->fields=NULL;
	batch->fieldcount=0;
	batch->multierror=false;
	batch->recordcounts=false;
	batch->detailederrors=BATCH_detailed_default;
	batch->buffersize=BATCH_hard_buffer_limit;
	batch->blobsegmented=false;
	batch->queuedbytes=0;
	batch->blobstreamblobid=0;
	batch->blobstreamremaining=0;
	batch->blobstreambpbremaining=0;
	batch->blobstreamsegremaining=0;
	batch->blobstreamsegmented=false;
	batch->blobstreambpb.clear();
	batch->blobstreamdata.clear();
}

void sqlrprotocol_firebird::clearBatch(sqlrfirebirdbatch *batch) {
	clearBatchMessages(batch);
	clearBatchBlobs(batch);
	delete[] batch->fields;
	initBatch(batch);
}

void sqlrprotocol_firebird::clearBatchMessages(sqlrfirebirdbatch *batch) {
	for (listnode< sqlrfirebirdbatchmessage * > *node=
					batch->messages.getFirst();
					node; node=node->getNext()) {
		sqlrfirebirdbatchmessage	*msg=node->getValue();
		for (uint16_t i=0; i<msg->valuecount; i++) {
			delete[] msg->values[i].strval;
		}
		delete[] msg->values;
		delete msg;
	}
	batch->messages.clear();
	batch->queuedbytes=0;
}

void sqlrprotocol_firebird::clearBatchBlobs(sqlrfirebirdbatch *batch) {

	// only the mapping goes - the blobs themselves belong to the session,
	// which trims them on its own budget (see trimBlobs()).  a blob the
	// batch streamed in was held against trimming while the batch could
	// still bind it, and nothing can now.
	for (listnode< sqlrfirebirdbatchblob * > *node=batch->blobs.getFirst();
						node; node=node->getNext()) {
		sqlrfirebirdbatchblob	*bb=node->getValue();
		sqlrfirebirdblob	*blob=getBlobById(0,bb->blobid);
		if (blob && !blob->handle) {
			blob->iswrite=false;
		}
		delete bb;
	}
	batch->blobs.clear();
	trimBlobs();
}

sqlrfirebirdbatchblob *sqlrprotocol_firebird::getBatchBlob(
						sqlrfirebirdbatch *batch,
						uint32_t high,
						uint32_t low) {
	for (listnode< sqlrfirebirdbatchblob * > *node=batch->blobs.getFirst();
						node; node=node->getNext()) {
		sqlrfirebirdbatchblob	*bb=node->getValue();
		if (bb->temphigh==high && bb->templow==low) {
			return bb;
		}
	}
	return NULL;
}

void sqlrprotocol_firebird::setBatchBlob(sqlrfirebirdbatch *batch,
						uint32_t high,
						uint32_t low,
						uint32_t blobid) {

	// re-registering an id replaces what it pointed at, which is what a
	// client that reuses one means
	sqlrfirebirdbatchblob	*bb=getBatchBlob(batch,high,low);
	if (!bb) {
		bb=new sqlrfirebirdbatchblob;
		bb->temphigh=high;
		bb->templow=low;
		batch->blobs.append(bb);
	}
	bb->blobid=blobid;
}

bool sqlrprotocol_firebird::parseBatchPb(const byte_t *pb,
					uint32_t pblen,
					sqlrfirebirdbatch *batch,
					uint32_t *gdscode) {

	*gdscode=0;

	// an empty parameter buffer just means the client asked for the
	// defaults
	if (!pb || !pblen) {
		return true;
	}

	const byte_t	*p=pb;
	const byte_t	*end=pb+pblen;

	// the version byte
	// (a real firebird refuses any version but this one rather than
	// guessing at the framing - server.cpp:3544-3546)
	byte_t	version=0;
	read(p,&version,&p);
	if (getDebug()) {
		stdoutput.printf("	batch pb version: %d\n",version);
	}
	if (version!=BATCH_version1) {
		*gdscode=isc_batch_param_version;
		return false;
	}

	// get each parameter...
	// (a batch parameter is a tag byte, a 4-byte little-endian value
	// length, and that many value bytes - unlike a dpb or bpb item, whose
	// length is a single byte)
	while ((size_t)(end-p)>4) {

		byte_t	tag=0;
		read(p,&tag,&p);

		uint32_t	valuelen=0;
		readLE(p,&valuelen,&p);

		if (getDebug()) {
			stdoutput.printf("	batch pb tag: %d, "
						"value length: %u\n",
						tag,valuelen);
		}

		// bail if the value runs past the end of the buffer
		if (valuelen>(size_t)(end-p)) {
			if (getDebug()) {
				stdoutput.write("	batch pb value runs "
						"past the end of the "
						"buffer\n");
			}
			break;
		}

		// every value is a little-endian integer
		uint32_t	value=0;
		for (uint32_t i=0; i<valuelen && i<4; i++) {
			value=value|(((uint32_t)p[i])<<(8*i));
		}

		p=p+valuelen;

		// process the parameter...
		switch (tag) {
			case BATCH_tag_multierror:
				batch->multierror=(value!=0);
				break;

			case BATCH_tag_record_counts:
				batch->recordcounts=(value!=0);
				break;

			case BATCH_tag_buffer_bytes_size:
				// 0 means no ceiling, which firebird's own
				// batch implements as the hard limit -
				// DsqlBatch.cpp:114-119
				batch->buffersize=
					(!value ||
					value>BATCH_hard_buffer_limit)?
						BATCH_hard_buffer_limit:value;
				break;

			case BATCH_tag_detailed_errors:
				batch->detailederrors=
					(value>BATCH_detailed_limit)?
						BATCH_detailed_limit:value;
				break;

			case BATCH_tag_blob_policy:
				// nothing to keep - a real firebird rewrites
				// the policy to BLOB_STREAM whatever the
				// client asked for, so a batch always streams
				// its blobs
				break;

			default:
				break;
		}
	}

	return true;
}

uint32_t sqlrprotocol_firebird::statementType(const char *query) {

	const char	*q=cont->skipWhitespaceAndComments(query);

	if (!charstring::compareIgnoringCase(q,"select",6)) {
		// a "for update" select is a different statement type, and a
		// client uses it to decide whether the cursor is updatable
		return (charstring::containsIgnoringCase(q," for update"))?
				isc_info_sql_stmt_select_for_upd:
				isc_info_sql_stmt_select;
	}
	if (!charstring::compareIgnoringCase(q,"insert",6)) {
		return isc_info_sql_stmt_insert;
	}
	if (!charstring::compareIgnoringCase(q,"update",6)) {
		return isc_info_sql_stmt_update;
	}
	if (!charstring::compareIgnoringCase(q,"delete",6)) {
		return isc_info_sql_stmt_delete;
	}
	if (!charstring::compareIgnoringCase(q,"execute procedure",17) ||
		!charstring::compareIgnoringCase(q,"exec ",5) ||
		!charstring::compareIgnoringCase(q,"call ",5)) {
		return isc_info_sql_stmt_exec_procedure;
	}
	if (!charstring::compareIgnoringCase(q,"commit",6)) {
		return isc_info_sql_stmt_commit;
	}
	if (!charstring::compareIgnoringCase(q,"rollback",8)) {
		return isc_info_sql_stmt_rollback;
	}
	if (!charstring::compareIgnoringCase(q,"set transaction",15)) {
		return isc_info_sql_stmt_start_trans;
	}
	if (!charstring::compareIgnoringCase(q,"savepoint",9)) {
		return isc_info_sql_stmt_savepoint;
	}
	if (!charstring::compareIgnoringCase(q,"set generator",13)) {
		return isc_info_sql_stmt_set_generator;
	}

	// create, alter, drop, grant, revoke and the rest
	// (this is a catch-all, so "execute block" and anything else the tests
	// above miss lands here too - a caller that has to know a statement
	// really is ddl asks ddlDynCode(), which matches the verb itself)
	return isc_info_sql_stmt_ddl;
}

bool sqlrprotocol_firebird::isTransactionStatement(uint32_t stmttype) {
	return (stmttype==isc_info_sql_stmt_start_trans ||
		stmttype==isc_info_sql_stmt_commit ||
		stmttype==isc_info_sql_stmt_rollback);
}

bool sqlrprotocol_firebird::isWriteStatement(uint32_t stmttype) {

	// only these three, and deliberately not ddl - statementType() uses
	// ddl as its catch-all, so "execute block", which a real server runs
	// read-only, lands there.  ddl is refused separately, by ddlDynCode(),
	// which recognizes the verb itself rather than trusting the catch-all,
	// and answers with the compound reply a real server sends.  "set
	// generator" is allowed anyway, since generators are outside
	// transaction control.
	return (stmttype==isc_info_sql_stmt_insert ||
		stmttype==isc_info_sql_stmt_update ||
		stmttype==isc_info_sql_stmt_delete);
}

// the ddl statements the module recognizes, keyed by the verb and the object
// type keyword that open them, each with the dyn code a real server refuses
// that statement with
// (this is the whole of what a read-only transaction refuses - a ddl statement
// that isn't here runs, the same way "execute block" and everything else in
// statementType()'s ddl catch-all runs.  left out on purpose: grant and revoke,
// whose templates take no object name and whose exact wire shape isn't
// confirmed here, "comment on", "alter character set", "create collation",
// "create package"/"create function" and their alter, drop and recreate forms,
// "drop generator", "declare external function", "alter database", "set
// statistics", and an index created with a "unique", "ascending" or
// "descending" modifier between the verb and the "index" keyword.  adding one
// is a row here plus its code above.)
struct sqlrfirebirdddlverb {
	const char	*verb;
	const char	*object;
	uint32_t	dyncode;
};

static const sqlrfirebirdddlverb sqlrfirebirdddlverbs[]={
	{"create","table",isc_dsql_create_table_failed},
	{"alter","table",isc_dsql_alter_table_failed},
	{"drop","table",isc_dsql_drop_table_failed},
	{"recreate","table",isc_dsql_recreate_table_failed},
	{"create or alter","view",isc_dsql_create_alter_view_failed},
	{"create","view",isc_dsql_create_view_failed},
	{"alter","view",isc_dsql_alter_view_failed},
	{"drop","view",isc_dsql_drop_view_failed},
	{"recreate","view",isc_dsql_recreate_view_failed},
	{"create or alter","procedure",isc_dsql_create_alter_proc_failed},
	{"create","procedure",isc_dsql_create_proc_failed},
	{"alter","procedure",isc_dsql_alter_proc_failed},
	{"drop","procedure",isc_dsql_drop_proc_failed},
	{"recreate","procedure",isc_dsql_recreate_proc_failed},
	{"create or alter","trigger",isc_dsql_create_alter_trigger_failed},
	{"create","trigger",isc_dsql_create_trigger_failed},
	{"alter","trigger",isc_dsql_alter_trigger_failed},
	{"drop","trigger",isc_dsql_drop_trigger_failed},
	{"recreate","trigger",isc_dsql_recreate_trigger_failed},
	{"create","domain",isc_dsql_create_domain_failed},
	{"alter","domain",isc_dsql_alter_domain_failed},
	{"drop","domain",isc_dsql_drop_domain_failed},
	{"create","exception",isc_dsql_create_except_failed},
	{"alter","exception",isc_dsql_alter_except_failed},
	{"drop","exception",isc_dsql_drop_except_failed},
	{"recreate","exception",isc_dsql_recreate_except_failed},
	{"create","sequence",isc_dsql_create_sequence_failed},
	{"alter","sequence",isc_dsql_alter_sequence_failed},
	{"drop","sequence",isc_dsql_drop_sequence_failed},
	{"recreate","sequence",isc_dsql_recreate_sequence_failed},
	{"create","generator",isc_dsql_create_generator_failed},
	{"create","index",isc_dsql_create_index_failed},
	{"alter","index",isc_dsql_alter_index_failed},
	{"drop","index",isc_dsql_drop_index_failed},
	{"create","role",isc_dsql_create_role_failed},
	{"alter","role",isc_dsql_alter_role_failed},
	{"drop","role",isc_dsql_drop_role_failed},
	{"create","user",isc_dsql_create_user_failed},
	{"alter","user",isc_dsql_alter_user_failed},
	{"drop","user",isc_dsql_drop_user_failed},
	{NULL,NULL,0}
};

const char *sqlrprotocol_firebird::matchDdlKeyword(const char *query,
						const char *keyword) {

	// a keyword can be several words - "create or alter" - and whitespace
	// or a comment can sit between any two of them
	const char	*q=query;
	const char	*k=keyword;
	while (*k) {

		size_t	len=0;
		while (k[len] && k[len]!=' ') {
			len++;
		}

		if (charstring::compareIgnoringCase(q,k,len)) {
			return NULL;
		}

		// the word in the query has to end where the keyword's does,
		// so "created" doesn't match "create"
		char	c=q[len];
		if (character::isAlphanumeric(c) || c=='_' || c=='$') {
			return NULL;
		}

		q=cont->skipWhitespaceAndComments(q+len);

		k=k+len;
		if (*k==' ') {
			k++;
		}
	}
	return q;
}

void sqlrprotocol_firebird::parseDdlObjectName(const char *query) {

	const char	*q=query;

	// a quoted name is the object's name verbatim, case and all, and an
	// unquoted one is upcased the way firebird upcases it
	bool	quoted=(*q=='"');
	if (quoted) {
		q++;
	}

	uint16_t	i=0;
	while (*q && i<FIREBIRD_MAX_OBJECT_NAME_LENGTH) {
		if (quoted) {
			if (*q=='"') {
				break;
			}
		} else if (!character::isAlphanumeric(*q) &&
					*q!='_' && *q!='$') {
			break;
		}
		ddlobjectname[i++]=*q++;
	}
	ddlobjectname[i]='\0';

	if (!quoted) {
		charstring::upper(ddlobjectname);
	}
}

bool sqlrprotocol_firebird::ddlDynCode(const char *query, uint32_t *dyncode) {

	ddlobjectname[0]='\0';

	const char	*q=cont->skipWhitespaceAndComments(query);

	for (const sqlrfirebirdddlverb *v=sqlrfirebirdddlverbs;
						v->verb; v++) {

		const char	*p=matchDdlKeyword(q,v->verb);
		if (!p) {
			continue;
		}
		p=matchDdlKeyword(p,v->object);
		if (!p) {
			continue;
		}

		// "if exists" or "if not exists" can sit between the object
		// type and the name
		const char	*e=matchDdlKeyword(p,"if not exists");
		if (!e) {
			e=matchDdlKeyword(p,"if exists");
		}
		if (e) {
			p=e;
		}

		parseDdlObjectName(p);

		*dyncode=v->dyncode;
		return true;
	}

	return false;
}

bool sqlrprotocol_firebird::readOnlyMetaUpdateResponse(const char *title,
						uint32_t dyncode,
						const char *objname) {

	// what a real server refuses ddl in a read-only transaction with - a
	// compound vector rather than the bare one an insert, update or delete
	// gets, and again nothing the client can't render itself
	// (the client turns the leading isc_no_meta_update into sqlcode -607,
	// sqlstate 42000 and "unsuccessful metadata update", the dyn code into
	// its own template with the object name substituted for @1, and
	// isc_read_only_trans into "attempted update during read-only
	// transaction" - three lines, out of its own tables, the same way it
	// gets one line out of a bare isc_read_only_trans)
	bytestring::zero(statusvector,sizeof(statusvector));
	bytestring::zero(statusvectorstr,sizeof(statusvectorstr));
	// the outer error...
	statusvector[0]=isc_arg_gds;
	statusvector[1]=isc_no_meta_update;
	// which ddl failed, and on what object...
	statusvector[2]=isc_arg_gds;
	statusvector[3]=dyncode;
	statusvector[4]=isc_arg_string;
	statusvectorstr[5]=objname;
	// why it failed...
	statusvector[6]=isc_arg_gds;
	statusvector[7]=isc_read_only_trans;
	// end of vector...
	statusvector[8]=isc_arg_end;
	statusvectorlen=9;

	return genericResponse(title,
				0,0,
				NULL,0,
				statusvector,statusvectorstr,
				statusvectorlen);
}

bool sqlrprotocol_firebird::runTransactionStatement(uint32_t stmttype) {

	// a client can ask for a transaction in sql rather than with
	// op_transaction - isql sends "set transaction" first and reads the
	// new handle out of the response
	// (these never reach the backend - SQL Relay drives its transaction
	// through the controller)
	switch (stmttype) {

		case isc_info_sql_stmt_start_trans:
			if (intransaction) {
				return true;
			}
			if (!cont->setAutoCommitOff() || !cont->begin()) {
				return false;
			}
			intransaction=true;
			trautocommit=false;
			trreadonly=false;
			trisolevel=0;
			trhandle++;
			return true;

		case isc_info_sql_stmt_commit:
			if (!intransaction) {
				return true;
			}
			if (!trautocommit && !cont->commit()) {
				return false;
			}
			clearStatements();
			intransaction=false;
			trautocommit=false;
			trreadonly=false;
			trisolevel=0;
			return true;

		case isc_info_sql_stmt_rollback:
			if (!intransaction) {
				return true;
			}
			if (!trautocommit && !cont->rollback()) {
				return false;
			}
			clearStatements();
			intransaction=false;
			trautocommit=false;
			trreadonly=false;
			trisolevel=0;
			return true;

		default:
			return true;
	}
}

void sqlrprotocol_firebird::keepReading(int32_t sec, int32_t usec) {
	// allow a short final read so a tail shorter than the buffer
	// still gets dumped, instead of timing out and being discarded
	bool	allowshort=clientsock->getAllowShortReads();
	clientsock->setAllowShortReads(true);
	for (;;) {
		byte_t	buffer[1024];
		ssize_t	r=clientsock->read(&buffer,sizeof(buffer),sec,usec);
		if (getDebug()) {
			stdoutput.printf("read %lld more bytes...\n",
							(long long)r);
		}
		if (r<1) {
			break;
		}
		debugHexDump(buffer,r);
	}
	clientsock->setAllowShortReads(allowshort);
}

void sqlrprotocol_firebird::readStringFromBuffer(const byte_t *in,
						uint32_t len,
						const char *name,
						char **buf) {

	// a dpb item may repeat, and the last one wins
	delete[] *buf;

	*buf=new char[len+1];
	bytestring::copy(*buf,in,len);
	(*buf)[len]='\0';
	if (getDebug()) {
		stdoutput.printf("	%s: %s\n",name,*buf);
	}
}

bool sqlrprotocol_firebird::readInt(uint32_t *val,
					const char *name,
					uint32_t *bytesread) {
	
	if (clientsock->read(val)!=sizeof(uint32_t)) {
		if (getDebug()) {
			stdoutput.printf("	read %s failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*bytesread)+=sizeof(uint32_t);
	if (getDebug()) {
		stdoutput.printf("	%s: %u\n",name,*val);
	}
	return true;
}

bool sqlrprotocol_firebird::readInt(uint32_t *val,
					const char *name,
					uint32_t expected,
					uint32_t *bytesread) {

	if (!readInt(val,name,bytesread)) {
		return false;
	}
	if (*val!=expected) {
		if (getDebug()) {
			stdoutput.printf("	invalid %s - "
						"got %u, expected %u\n",
						name,*val,expected);
			debugEnd();
		}
		return false;
	}
	return true;
}

uint32_t sqlrprotocol_firebird::foldSignExtendedLength(uint32_t len,
							const char *name) {

	// these fields were 16 bit shorts in older versions of the protocol,
	// so an older client sends 32768 or more sign-extended into the 32 bit
	// field, and firebird folds it back down wherever it appears - see
	// fixupLength() in its src/remote/protocol.cpp
	// (this has to run before any cap or reject, because 0xffff8000 means
	// 32768, and capping or rejecting first would hand that client a 65535
	// ceiling)
	if ((len&0xffff0000)==0xffff0000) {
		if (getDebug()) {
			stdoutput.printf("	folded sign-extended %s "
						"length - got %u, "
						"folded to %u\n",
						name,len,len&0xffff);
		}
		len&=0xffff;
	}
	return len;
}

bool sqlrprotocol_firebird::readString(char **val,
					const char *name,
					uint32_t *bytesread) {
	return readString(val,NULL,name,bytesread);
}

bool sqlrprotocol_firebird::readString(char **val,
					uint32_t *len,
					const char *name,
					uint32_t *bytesread) {

	// init buffer up front, so a failure below never leaves the caller
	// the pointer it had before (attach() delete[]'s db before this call)
	*val=NULL;
	if (len) {
		*len=0;
	}

	// read length
	uint32_t	vallen=0;
	if (clientsock->read(&vallen)!=sizeof(uint32_t)) {
		if (getDebug()) {
			stdoutput.printf("	read %s length failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	if (getDebug()) {
		stdoutput.printf("	%s length: %u\n",name,vallen);
	}
	(*bytesread)+=sizeof(uint32_t);

	vallen=foldSignExtendedLength(vallen,name);

	// reject an out-of-bounds length, before it can size an allocation
	// (vallen+1 would also wrap to 0 at 0xffffffff)
	// (the fold above already ran, so an older client's sign-extended
	// length lands in range here, and only a genuinely out-of-range
	// length gets rejected)
	if (vallen>MAX_CSTRING_LENGTH) {
		if (getDebug()) {
			stdoutput.printf("	invalid %s length - "
						"got %u, max %u\n",
						name,vallen,
						(uint32_t)MAX_CSTRING_LENGTH);
			debugEnd();
		}
		return false;
	}

	if (vallen) {

		// allocate buffer
		*val=new char[vallen+1];

		// read buffer
		if (clientsock->read(*val,vallen)!=(ssize_t)vallen) {
			if (getDebug()) {
				stdoutput.printf("	read %s failed\n",name);
				debugSystemError();
				debugEnd();
			}
			delete[] *val;
			*val=NULL;
			return false;
		}
		(*val)[vallen]='\0';
		if (getDebug()) {
			stdoutput.printf("	%s: %s\n",name,*val);
		}
		(*bytesread)+=vallen;
	}

	if (len) {
		*len=vallen;
	}

	// read padding
	if (!readPadding(bytesread)) {
		delete[] *val;
		*val=NULL;
		if (len) {
			*len=0;
		}
		return false;
	}
	return true;
}

bool sqlrprotocol_firebird::readBuffer(byte_t **val,
					const char *name,
					uint32_t *bytesread) {
	return readBuffer(val,NULL,name,bytesread);
}

bool sqlrprotocol_firebird::readBuffer(byte_t **val,
					uint32_t *len,
					const char *name,
					uint32_t *bytesread) {

	// init buffer up front, so a failure below never leaves the caller
	// the pointer it had before
	*val=NULL;
	if (len) {
		*len=0;
	}

	// read length
	uint32_t	vallen=0;
	if (clientsock->read(&vallen)!=sizeof(uint32_t)) {
		if (getDebug()) {
			stdoutput.printf("	read %s length failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	if (getDebug()) {
		stdoutput.printf("	%s length: %u\n",name,vallen);
	}
	(*bytesread)+=sizeof(uint32_t);

	vallen=foldSignExtendedLength(vallen,name);

	// reject an out-of-bounds length, before it can size an allocation
	// (the fold above already ran, so an older client's sign-extended
	// length lands in range here, and only a genuinely out-of-range
	// length gets rejected)
	if (vallen>MAX_CSTRING_LENGTH) {
		if (getDebug()) {
			stdoutput.printf("	invalid %s length - "
						"got %u, max %u\n",
						name,vallen,
						(uint32_t)MAX_CSTRING_LENGTH);
			debugEnd();
		}
		return false;
	}

	if (vallen) {

		// allocate buffer
		*val=new byte_t[vallen];

		// read buffer
		if (clientsock->read(*val,vallen)!=(ssize_t)vallen) {
			if (getDebug()) {
				stdoutput.printf("	read %s failed\n",name);
				debugSystemError();
				debugEnd();
			}
			delete[] *val;
			*val=NULL;
			return false;
		}
		if (getDebug()) {
			stdoutput.printf("	%s:\n",name);
			stdoutput.printHex(*val,vallen);
		}
		(*bytesread)+=vallen;
	}

	if (len) {
		*len=vallen;
	}

	// read padding
	if (!readPadding(bytesread)) {
		delete[] *val;
		*val=NULL;
		if (len) {
			*len=0;
		}
		return false;
	}
	return true;
}

bool sqlrprotocol_firebird::readBytes(bytebuffer *val,
					uint32_t len,
					const char *name,
					uint32_t *bytesread) {

	// read in chunks, since the caller's length is whatever the client
	// said, and appending as we go keeps a piece that arrived in an
	// earlier packet in front of this one
	byte_t		buffer[8192];
	uint32_t	total=len;
	while (len) {
		uint32_t	chunk=(len<sizeof(buffer))?len:sizeof(buffer);
		if (clientsock->read(buffer,chunk)!=(ssize_t)chunk) {
			if (getDebug()) {
				stdoutput.printf("	read %s failed\n",name);
				debugSystemError();
				debugEnd();
			}
			return false;
		}
		val->append(buffer,chunk);
		(*bytesread)+=chunk;
		len=len-chunk;
	}
	if (getDebug()) {
		stdoutput.printf("	%s: %u byte(s)\n",name,total);
	}
	return true;
}

bool sqlrprotocol_firebird::readPadding(uint32_t *bytesread) {

	// handle degenerate case
	if (!(*bytesread%4)) {
		if (getDebug()) {
			stdoutput.write("	(0 bytes of padding)\n");
		}
		return true;
	}

	// how much padding do we need to read?
	// (pad to a 4-byte boundary)
	uint32_t	pad=((((*bytesread)/4)+1)*4)-(*bytesread);

	// read the padding
	byte_t		dummy;
	for (uint32_t i=0; i<pad; i++) {
		if (clientsock->read(&dummy)!=sizeof(byte_t)) {
			if (getDebug()) {
				stdoutput.write("	read padding failed\n");
				debugSystemError();
				debugEnd();
			}
			return false;
		}
		(*bytesread)++;
	}
	if (getDebug()) {
		stdoutput.printf("	(%u bytes of padding)\n",pad);
	}
	return true;
}

void sqlrprotocol_firebird::fixupRespBufferLen() {

	respbufferlen=foldSignExtendedLength(respbufferlen,"response buffer");

	// the length never sizes an allocation here - it's only the ceiling
	// that truncates the response - but a client that declares a huge one
	// never truncates, so the response buffer grows to whatever the
	// requested items produce
	// (firebird's api types this length as a short, so a cap costs a real
	// client nothing, and clamping rather than failing leaves the
	// protocol's own truncation as the answer to too large an ask.
	// firebird itself allocates whatever the client declares, twice for
	// op_info_database, with no ceiling - see rem_port::info() in its
	// src/remote/server/server.cpp.)
	if (respbufferlen<=MAX_CSTRING_LENGTH) {
		return;
	}

	if (getDebug()) {
		stdoutput.printf("	capped response buffer length - "
					"got %u, max %u\n",
					respbufferlen,
					(uint32_t)MAX_CSTRING_LENGTH);
	}

	respbufferlen=MAX_CSTRING_LENGTH;
}

bool sqlrprotocol_firebird::writeInt(uint32_t val,
					const char *name,
					uint32_t *byteswritten) {
	
	if (clientsock->write(val)!=sizeof(uint32_t)) {
		if (getDebug()) {
			stdoutput.printf("	write %s failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*byteswritten)+=sizeof(uint32_t);
	if (getDebug()) {
		stdoutput.printf("	%s: %u\n",name,val);
	}
	return true;
}

bool sqlrprotocol_firebird::writeBuffer(const byte_t *val,
					uint32_t len,
					const char *name,
					uint32_t *byteswritten) {

	// write length
	// (the length on the wire is the length without the padding below)
	if (clientsock->write(len)!=sizeof(uint32_t)) {
		if (getDebug()) {
			stdoutput.printf("	write %s length failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	if (getDebug()) {
		stdoutput.printf("	%s len: %u\n",name,len);
	}
	(*byteswritten)+=sizeof(uint32_t);

	// write buffer
	if (clientsock->write(val,len)!=(ssize_t)len) {
		if (getDebug()) {
			stdoutput.printf("	write %s failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	if (getDebug()) {
		stdoutput.printf("	%s:\n",name);
		stdoutput.printHex(val,len);
	}
	(*byteswritten)+=len;

	// write padding
	// (pad to a 4-byte boundary)
	uint32_t	pad=(4-len)&3;
	byte_t		zero[3]={0,0,0};
	if (pad && clientsock->write(zero,pad)!=(ssize_t)pad) {
		if (getDebug()) {
			stdoutput.printf("	write %s padding failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*byteswritten)+=pad;
	if (getDebug()) {
		stdoutput.printf("	(%u bytes of padding)\n",pad);
	}
	return true;
}

bool sqlrprotocol_firebird::writeStatusVector(uint32_t *sv,
					const char **svstr,
					uint8_t svlen,
					uint32_t *byteswritten) {

	// an argument string is written the same way any other buffer is - a
	// length, the bytes, and padding to a 4-byte boundary
	for (uint8_t i=0; i<svlen; i++) {
		if (svstr[i]) {
			if (!writeBuffer((const byte_t *)svstr[i],
					charstring::getLength(svstr[i]),
					"status vector string",
					byteswritten)) {
				return false;
			}
			continue;
		}
		if (clientsock->write(sv[i])!=sizeof(uint32_t)) {
			if (getDebug()) {
				stdoutput.printf("	write status "
						"vector [%u] failed\n",i);
				debugSystemError();
				debugEnd();
			}
			return false;
		}
		(*byteswritten)+=sizeof(uint32_t);
	}
	if (getDebug()) {
		debugStatusVector(sv,svstr,svlen);
	}
	return true;
}

bool sqlrprotocol_firebird::writeInt64(uint64_t val,
					const char *name,
					uint32_t *byteswritten) {

	if (clientsock->write(val)!=sizeof(uint64_t)) {
		if (getDebug()) {
			stdoutput.printf("	write %s failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*byteswritten)+=sizeof(uint64_t);
	return true;
}

bool sqlrprotocol_firebird::readInt64(uint64_t *val,
					const char *name,
					uint32_t *bytesread) {

	if (clientsock->read(val)!=sizeof(uint64_t)) {
		if (getDebug()) {
			stdoutput.printf("	read %s failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*bytesread)+=sizeof(uint64_t);
	return true;
}

bool sqlrprotocol_firebird::writeOpaque(const byte_t *val,
					uint32_t len,
					const char *name,
					uint32_t *byteswritten) {

	// a message field carries no length of its own - the blr already said
	// how long it is - but it is still padded to a 4-byte boundary
	if (len && clientsock->write(val,len)!=(ssize_t)len) {
		if (getDebug()) {
			stdoutput.printf("	write %s failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*byteswritten)+=len;

	uint32_t	pad=(4-len)&3;
	byte_t		zero[3]={0,0,0};
	if (pad && clientsock->write(zero,pad)!=(ssize_t)pad) {
		if (getDebug()) {
			stdoutput.printf("	write %s padding failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*byteswritten)+=pad;
	return true;
}

bool sqlrprotocol_firebird::readOpaque(byte_t *val,
					uint32_t len,
					const char *name,
					uint32_t *bytesread) {

	if (len && clientsock->read(val,len)!=(ssize_t)len) {
		if (getDebug()) {
			stdoutput.printf("	read %s failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*bytesread)+=len;

	uint32_t	pad=(4-len)&3;
	byte_t		dummy[3];
	if (pad && clientsock->read(dummy,pad)!=(ssize_t)pad) {
		if (getDebug()) {
			stdoutput.printf("	read %s padding failed\n",name);
			debugSystemError();
			debugEnd();
		}
		return false;
	}
	(*bytesread)+=pad;
	return true;
}

bool sqlrprotocol_firebird::readBlr(sqlrfirebirdfield **fields,
					uint16_t *fieldcount,
					const char *name,
					uint32_t *bytesread,
					uint32_t *gdscode) {

	*fields=NULL;
	*fieldcount=0;
	*gdscode=0;

	uint32_t	blrlen=0;
	byte_t		*blr=NULL;
	if (!readBuffer(&blr,&blrlen,name,bytesread)) {
		// a real socket failure, not a malformed blr - nothing to
		// answer, the connection is already gone
		return false;
	}

	bool	retval=parseBlr(blr,blrlen,fields,fieldcount,gdscode);

	delete[] blr;

	return retval;
}

bool sqlrprotocol_firebird::parseBlr(const byte_t *blr,
					uint32_t blrlen,
					sqlrfirebirdfield **fields,
					uint16_t *fieldcount,
					uint32_t *gdscode) {

	// blr data structure:
	//
	// data {
	// 	byte_t		blr version
	// 	byte_t		blr_begin
	// 	byte_t		blr_message
	// 	byte_t		message number
	// 	uint16_t	item count, little-endian
	// 	byte_t[]	items
	// 	byte_t		blr_end
	// 	byte_t		blr_eoc
	// }
	//
	// Each column contributes two items - the value and a null indicator -
	// so the item count is twice the column count.

	*fields=NULL;
	*fieldcount=0;
	// 0 means the caller should just close - anything else is a real
	// firebird error code the caller should answer with
	*gdscode=0;

	// an empty blr just means the client had nothing to describe
	if (!blr || blrlen<6) {
		return true;
	}

	const byte_t	*p=blr;
	const byte_t	*end=blr+blrlen;

	byte_t	version=*p;
	p++;
	if (version!=blr_version4 && version!=blr_version5) {
		if (getDebug()) {
			stdoutput.printf("	invalid blr version: %u\n",
								version);
		}
		*gdscode=isc_dsql_error;
		return false;
	}
	if (*p!=blr_begin || *(p+1)!=blr_message) {
		if (getDebug()) {
			stdoutput.write("	invalid blr message header\n");
		}
		*gdscode=isc_dsql_error;
		return false;
	}
	p+=2;

	// message number
	p++;

	uint16_t	itemcount=(uint16_t)(p[0]|(p[1]<<8));
	p+=2;

	// the count is always even - an odd one off the wire would round the
	// array down and leave the last item writing past its end
	if (itemcount%2) {
		if (getDebug()) {
			stdoutput.printf("	odd blr item count: %u\n",
								itemcount);
		}
		*gdscode=isc_dsql_error;
		return false;
	}

	return parseBlrItems(&p,end,itemcount,true,fields,fieldcount,gdscode);
}

bool sqlrprotocol_firebird::parseBlrItems(const byte_t **blr,
					const byte_t *end,
					uint16_t itemcount,
					bool paired,
					sqlrfirebirdfield **fields,
					uint16_t *fieldcount,
					uint32_t *gdscode) {

	// A message blr pairs each value with a null indicator, and only the
	// value describes a field.  A request blr's message lists every item
	// on its own instead, indicator or not, so every one of them is a
	// field.

	*fields=NULL;
	*fieldcount=0;

	uint16_t	count=(paired)?itemcount/2:itemcount;
	if (!count) {
		return true;
	}

	const byte_t		*p=*blr;
	sqlrfirebirdfield	*f=new sqlrfirebirdfield[count];

	for (uint16_t i=0; i<itemcount; i++) {

		if (p>=end) {
			delete[] f;
			*gdscode=isc_dsql_error;
			return false;
		}

		sqlrfirebirdfield	fld;
		fld.blrtype=*p;
		fld.scale=0;
		fld.subtype=0;
		fld.length=0;
		p++;

		switch (fld.blrtype) {
			case blr_text:
			case blr_varying:
			case blr_cstring:
				if (p+2>end) {
					delete[] f;
					*gdscode=isc_dsql_error;
					return false;
				}
				fld.length=(uint16_t)(p[0]|(p[1]<<8));
				p+=2;
				break;

			case blr_text2:
			case blr_varying2:
			case blr_cstring2:
				if (p+4>end) {
					delete[] f;
					*gdscode=isc_dsql_error;
					return false;
				}
				fld.subtype=(uint16_t)(p[0]|(p[1]<<8));
				fld.length=(uint16_t)(p[2]|(p[3]<<8));
				p+=4;
				break;

			case blr_blob2:
				// a blob's two words are its sub type and its
				// scale, not a length
				if (p+4>end) {
					delete[] f;
					*gdscode=isc_dsql_error;
					return false;
				}
				fld.subtype=(uint16_t)(p[0]|(p[1]<<8));
				fld.scale=(int16_t)(p[2]|(p[3]<<8));
				p+=4;
				break;

			case blr_short:
			case blr_long:
			case blr_quad:
			case blr_int64:
				if (p>=end) {
					delete[] f;
					*gdscode=isc_dsql_error;
					return false;
				}
				fld.scale=(int16_t)((int8_t)*p);
				p++;
				break;

			case blr_float:
			case blr_double:
			case blr_d_float:
			case blr_sql_date:
			case blr_sql_time:
			case blr_timestamp:
			case blr_bool:
				break;

			default:
				if (getDebug()) {
					stdoutput.printf("	unrecognized "
							"blr item: %u\n",
							fld.blrtype);
				}
				delete[] f;
				*gdscode=isc_dsql_datatype_err;
				return false;
		}

		// the odd items of a paired list are the null indicators,
		// which are always a blr_short, and carry nothing the module
		// needs
		if (paired) {
			if (!(i%2)) {
				f[i/2]=fld;
			}
		} else {
			f[i]=fld;
		}
	}

	*blr=p;
	*fields=f;
	*fieldcount=count;

	if (getDebug()) {
		stdoutput.printf("	blr describes %u field(s)\n",count);
	}

	return true;
}

void sqlrprotocol_firebird::initBlrRequest(sqlrfirebirdblrrequest *req) {
	req->compiled=false;
	for (uint16_t i=0; i<FIREBIRD_MAX_BLR_CONTEXTS; i++) {
		req->relations[i]=NULL;
	}
	req->relationcount=0;
	for (uint16_t i=0; i<FIREBIRD_MAX_BLR_MESSAGES; i++) {
		req->msgfields[i]=NULL;
		req->msgfieldcount[i]=0;
	}
	req->outmsg=0;
	req->inmsg=0;
	req->hasinmsg=false;
	req->rowslots=NULL;
	req->eofslots=NULL;
	req->slotcount=0;
	req->haseof=false;
	req->inparamcount=0;
	req->query=NULL;
	req->querylen=0;
}

void sqlrprotocol_firebird::clearBlrRequest(sqlrfirebirdblrrequest *req) {
	for (uint16_t i=0; i<FIREBIRD_MAX_BLR_CONTEXTS; i++) {
		delete[] req->relations[i];
	}
	for (uint16_t i=0; i<FIREBIRD_MAX_BLR_MESSAGES; i++) {
		delete[] req->msgfields[i];
	}
	clearBlrSlots(req->rowslots,req->slotcount);
	clearBlrSlots(req->eofslots,req->slotcount);
	delete[] req->query;
	initBlrRequest(req);
}

sqlrfirebirdblrslot *sqlrprotocol_firebird::newBlrSlots(uint16_t count) {
	sqlrfirebirdblrslot	*slots=new sqlrfirebirdblrslot[count];
	for (uint16_t i=0; i<count; i++) {
		slots[i].column=-1;
		slots[i].literal=NULL;
		slots[i].indicator=-1;
	}
	return slots;
}

void sqlrprotocol_firebird::clearBlrSlots(sqlrfirebirdblrslot *slots,
						uint16_t count) {
	for (uint16_t i=0; i<count; i++) {
		delete[] slots[i].literal;
	}
	delete[] slots;
}

bool sqlrprotocol_firebird::parseBlrRequest(const byte_t *blr,
					uint32_t blrlen,
					sqlrfirebirdblrrequest *req,
					uint32_t *gdscode) {

	// request blr data structure:
	//
	// data {
	// 	byte_t		blr version
	// 	byte_t[]	statement
	// 	byte_t		blr_eoc
	// }
	//
	// The server API takes sql and nothing else, so rather than run the
	// request the module translates it to a select and runs that.  Only
	// the shape isql's SHOW commands compile is understood - a begin
	// block declaring a message per parameter set, a receive around the
	// body when the request takes input parameters, a for loop over a
	// one or two relation rse with a send inside it, and a second send
	// after the loop whose message tells the client the stream ended.
	// Anything else is refused rather than guessed at, since a request
	// half understood would answer rows that aren't the ones it asked
	// for.

	*gdscode=0;

	clearBlrRequest(req);

	if (!blr || blrlen<2) {
		*gdscode=isc_dsql_error;
		return false;
	}

	const byte_t	*p=blr;
	const byte_t	*end=blr+blrlen;

	byte_t	version=*p;
	p++;
	if (version!=blr_version4 && version!=blr_version5) {
		if (getDebug()) {
			stdoutput.printf("	invalid blr version: %u\n",
								version);
		}
		*gdscode=isc_dsql_error;
		return false;
	}

	sqlrfirebirdblrwalk	walk;
	walk.selectcount=0;
	walk.inloop=false;
	walk.slots=NULL;
	walk.depth=0;

	if (!parseBlrStatement(&p,end,req,&walk,gdscode) ||
			!buildBlrRequestQuery(req,&walk,gdscode)) {
		clearBlrRequest(req);
		return false;
	}

	req->compiled=true;

	return true;
}

bool sqlrprotocol_firebird::parseBlrStatement(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode) {

	// statement verbs:
	//
	// blr_begin		<statement>... blr_end
	// blr_message		<message number> <item count> <items>
	// blr_receive		<message number> <statement>
	// blr_for		<rse> <statement>
	// blr_send		<message number> <statement>
	// blr_assignment	<value> <parameter>

	if (walk->depth>=FIREBIRD_MAX_BLR_DEPTH) {
		*gdscode=isc_wish_list;
		return false;
	}
	walk->depth++;

	const byte_t	*p=*blr;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}

	byte_t	verb=*p;
	p++;

	switch (verb) {

		case blr_begin:
			for (;;) {
				if (p>=end) {
					*gdscode=isc_dsql_error;
					return false;
				}
				if (*p==blr_end) {
					p++;
					break;
				}
				*blr=p;
				if (!parseBlrStatement(blr,end,req,
							walk,gdscode)) {
					return false;
				}
				p=*blr;
			}
			break;

		case blr_message:
			{
			if (p+3>end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			byte_t	msgnumber=*p;
			p++;
			uint16_t	itemcount=(uint16_t)(p[0]|(p[1]<<8));
			p+=2;
			if (msgnumber>=FIREBIRD_MAX_BLR_MESSAGES ||
					req->msgfields[msgnumber]) {
				*gdscode=isc_wish_list;
				return false;
			}
			if (!parseBlrItems(&p,end,itemcount,false,
					&(req->msgfields[msgnumber]),
					&(req->msgfieldcount[msgnumber]),
					gdscode)) {
				return false;
			}
			}
			break;

		case blr_receive:
			{
			if (p>=end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			byte_t	msgnumber=*p;
			p++;
			// what the request receives is what the client sends
			// with op_start_send_and_receive
			if (msgnumber>=FIREBIRD_MAX_BLR_MESSAGES ||
					!req->msgfields[msgnumber]) {
				*gdscode=isc_wish_list;
				return false;
			}
			req->inmsg=msgnumber;
			req->hasinmsg=true;
			*blr=p;
			if (!parseBlrStatement(blr,end,req,walk,gdscode)) {
				return false;
			}
			p=*blr;
			}
			break;

		case blr_for:
			{
			*blr=p;
			if (!parseBlrRse(blr,end,req,walk,gdscode)) {
				return false;
			}
			walk->inloop=true;
			bool	parsed=parseBlrStatement(blr,end,req,
							walk,gdscode);
			walk->inloop=false;
			if (!parsed) {
				return false;
			}
			p=*blr;
			}
			break;

		case blr_send:
			{
			if (p>=end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			byte_t	msgnumber=*p;
			p++;
			if (msgnumber>=FIREBIRD_MAX_BLR_MESSAGES ||
					!req->msgfields[msgnumber]) {
				*gdscode=isc_wish_list;
				return false;
			}
			// the send inside the loop is what carries a row -
			// the one after it fills the same message with
			// whatever tells the client the rows ran out
			if (walk->inloop) {
				if (req->rowslots) {
					*gdscode=isc_wish_list;
					return false;
				}
				req->outmsg=msgnumber;
				req->slotcount=req->msgfieldcount[msgnumber];
				req->rowslots=newBlrSlots(req->slotcount);
				req->eofslots=newBlrSlots(req->slotcount);
				walk->slots=req->rowslots;
			} else {
				if (!req->rowslots ||
					msgnumber!=req->outmsg ||
					req->haseof) {
					*gdscode=isc_wish_list;
					return false;
				}
				req->haseof=true;
				walk->slots=req->eofslots;
			}
			*blr=p;
			bool	parsed=parseBlrStatement(blr,end,req,
							walk,gdscode);
			walk->slots=NULL;
			if (!parsed) {
				return false;
			}
			p=*blr;
			}
			break;

		case blr_assignment:
			*blr=p;
			if (!parseBlrAssignment(blr,end,req,walk,gdscode)) {
				return false;
			}
			p=*blr;
			break;

		default:
			if (getDebug()) {
				stdoutput.printf("	unsupported blr "
						"statement verb: %u\n",verb);
			}
			*gdscode=isc_wish_list;
			return false;
	}

	walk->depth--;

	*blr=p;

	return true;
}

bool sqlrprotocol_firebird::parseBlrAssignment(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode) {

	// blr_assignment <source value> <destination parameter>

	stringbuffer	sql;
	stringbuffer	raw;
	byte_t		kind=0;
	if (!parseBlrValue(blr,end,req,&sql,&raw,&kind,gdscode)) {
		return false;
	}

	const byte_t	*p=*blr;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}

	// a request only ever assigns into its own message
	byte_t	verb=*p;
	p++;
	if (verb!=blr_parameter && verb!=blr_parameter2) {
		*gdscode=isc_wish_list;
		return false;
	}

	if (p+3>end) {
		*gdscode=isc_dsql_error;
		return false;
	}
	byte_t	msgnumber=*p;
	p++;
	uint16_t	paramnumber=(uint16_t)(p[0]|(p[1]<<8));
	p+=2;
	bool		hasindicator=(verb==blr_parameter2);
	uint16_t	indicatornumber=0;
	if (hasindicator) {
		// the value's null indicator gets an item of the message all
		// its own
		if (p+2>end) {
			*gdscode=isc_dsql_error;
			return false;
		}
		indicatornumber=(uint16_t)(p[0]|(p[1]<<8));
		p+=2;
	}

	*blr=p;

	if (!walk->slots || msgnumber!=req->outmsg ||
			paramnumber>=req->slotcount ||
			(hasindicator && indicatornumber>=req->slotcount)) {
		*gdscode=isc_wish_list;
		return false;
	}

	sqlrfirebirdblrslot	*slot=&(walk->slots[paramnumber]);

	if (kind==blr_field) {
		// a column can only come from a row, and only the send inside
		// the loop has one
		if (!walk->inloop) {
			*gdscode=isc_wish_list;
			return false;
		}
		if (walk->selectcount) {
			walk->selectclause.append(",");
		}
		walk->selectclause.append(sql.getString());
		slot->column=(int32_t)walk->selectcount;
		// the indicator item reports on the same column the value
		// came from
		if (hasindicator) {
			walk->slots[indicatornumber].indicator=slot->column;
		}
		walk->selectcount++;
	} else if (kind==blr_literal) {
		delete[] slot->literal;
		slot->literal=charstring::duplicate(raw.getString());
	} else if (kind!=blr_null) {
		*gdscode=isc_wish_list;
		return false;
	}

	return true;
}

bool sqlrprotocol_firebird::parseBlrRse(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode) {

	// blr_rse <relation count> <relation>... <clause>... blr_end
	//
	// A relation's position in the list is not what a field reference
	// names it by - each relation carries a context number of its own,
	// and that is what blr_field uses.

	const byte_t	*p=*blr;

	if (p>=end || *p!=blr_rse) {
		*gdscode=isc_wish_list;
		return false;
	}
	p++;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}
	byte_t	relcount=*p;
	p++;

	// two relations is an inner join, and is as wide as the requests
	// isql compiles ever get - the join condition is just another
	// predicate in the boolean, so a comma join says exactly what the
	// rse says
	if (!relcount || relcount>FIREBIRD_MAX_BLR_RELATIONS) {
		*gdscode=isc_wish_list;
		return false;
	}

	for (byte_t i=0; i<relcount; i++) {
		*blr=p;
		if (!parseBlrRelation(blr,end,req,walk,gdscode)) {
			return false;
		}
		p=*blr;
	}

	for (;;) {

		if (p>=end) {
			*gdscode=isc_dsql_error;
			return false;
		}

		byte_t	verb=*p;
		if (verb==blr_end) {
			p++;
			break;
		}
		p++;

		switch (verb) {

			case blr_boolean:
				*blr=p;
				if (!parseBlrBoolean(blr,end,req,
						&walk->whereclause,
						0,gdscode)) {
					return false;
				}
				p=*blr;
				break;

			case blr_first:
				{
				// the ?s bind in the order they appear in
				// the query, and the row limit ends up at
				// the end of it rather than where the rse
				// puts it, so a parameter here would bind
				// out of turn
				uint16_t	inparamcount=
							req->inparamcount;
				*blr=p;
				if (!parseBlrValue(blr,end,req,
						&walk->firstclause,
						NULL,NULL,gdscode)) {
					return false;
				}
				if (req->inparamcount!=inparamcount) {
					*gdscode=isc_wish_list;
					return false;
				}
				p=*blr;
				}
				break;

			case blr_sort:
				*blr=p;
				if (!parseBlrSort(blr,end,req,walk,gdscode)) {
					return false;
				}
				p=*blr;
				break;

			case blr_join_type:
				// the only join two relations in one rse can
				// express is an inner one, which is what the
				// from clause's comma join already is
				if (p>=end) {
					*gdscode=isc_dsql_error;
					return false;
				}
				p++;
				break;

			default:
				if (getDebug()) {
					stdoutput.printf("	unsupported "
							"blr rse clause: "
							"%u\n",verb);
				}
				*gdscode=isc_wish_list;
				return false;
		}
	}

	*blr=p;

	return true;
}

bool sqlrprotocol_firebird::parseBlrRelation(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode) {

	// blr_relation	 <name length> <name> <context>
	// blr_relation2 <name length> <name> <alias length> <alias> <context>

	const byte_t	*p=*blr;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}

	byte_t	verb=*p;
	p++;
	if (verb!=blr_relation && verb!=blr_relation2) {
		if (getDebug()) {
			stdoutput.printf("	unsupported blr relation "
						"verb: %u\n",verb);
		}
		*gdscode=isc_wish_list;
		return false;
	}

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}
	byte_t	namelen=*p;
	p++;
	if (p+namelen>end) {
		*gdscode=isc_dsql_error;
		return false;
	}
	const byte_t	*name=p;
	p+=namelen;

	// the alias the request gave it doesn't survive the translation -
	// see below
	if (verb==blr_relation2) {
		if (p>=end) {
			*gdscode=isc_dsql_error;
			return false;
		}
		byte_t	aliaslen=*p;
		p++;
		if (p+aliaslen>end) {
			*gdscode=isc_dsql_error;
			return false;
		}
		p+=aliaslen;
	}

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}
	byte_t	context=*p;
	p++;

	if (context>=FIREBIRD_MAX_BLR_CONTEXTS || req->relations[context]) {
		*gdscode=isc_wish_list;
		return false;
	}

	req->relations[context]=charstring::duplicate((const char *)name,
								namelen);

	// the alias the from clause gives it is the context number, which is
	// what a field reference names it by anyway
	if (req->relationcount) {
		walk->fromclause.append(",");
	}
	walk->fromclause.append(req->relations[context])->
				append(" C")->append((uint32_t)context);

	req->relationcount++;

	*blr=p;

	return true;
}

bool sqlrprotocol_firebird::parseBlrSort(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode) {

	// blr_sort <key count> (<direction> <value>)...

	const byte_t	*p=*blr;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}
	byte_t	keycount=*p;
	p++;

	for (byte_t i=0; i<keycount; i++) {

		if (p>=end) {
			*gdscode=isc_dsql_error;
			return false;
		}

		bool	descending=false;
		if (*p==blr_ascending || *p==blr_descending) {
			descending=(*p==blr_descending);
			p++;
		}

		if (i) {
			walk->orderbyclause.append(",");
		}

		// a parameter here would bind out of turn - see the first
		// clause in parseBlrRse()
		uint16_t	inparamcount=req->inparamcount;

		*blr=p;
		if (!parseBlrValue(blr,end,req,&walk->orderbyclause,
						NULL,NULL,gdscode)) {
			return false;
		}
		if (req->inparamcount!=inparamcount) {
			*gdscode=isc_wish_list;
			return false;
		}
		p=*blr;

		if (descending) {
			walk->orderbyclause.append(" DESC");
		}
	}

	*blr=p;

	return true;
}

bool sqlrprotocol_firebird::parseBlrBoolean(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					stringbuffer *sql,
					uint16_t depth,
					uint32_t *gdscode) {

	// boolean verbs:
	//
	// blr_and, blr_or			<boolean> <boolean>
	// blr_not				<boolean>
	// blr_missing				<value>
	// blr_between				<value> <value> <value>
	// blr_eql and the other comparisons	<value> <value>

	if (depth>=FIREBIRD_MAX_BLR_DEPTH) {
		*gdscode=isc_wish_list;
		return false;
	}

	const byte_t	*p=*blr;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}

	byte_t	verb=*p;
	p++;
	*blr=p;

	const char	*op=NULL;
	switch (verb) {
		case blr_and:
			op=" AND ";
			break;
		case blr_or:
			op=" OR ";
			break;
		case blr_eql:
			op="=";
			break;
		case blr_neq:
			op="<>";
			break;
		case blr_gtr:
			op=">";
			break;
		case blr_geq:
			op=">=";
			break;
		case blr_lss:
			op="<";
			break;
		case blr_leq:
			op="<=";
			break;
		case blr_like:
			op=" LIKE ";
			break;
		case blr_starting:
			op=" STARTING WITH ";
			break;
		case blr_containing:
			op=" CONTAINING ";
			break;
		default:
			break;
	}

	switch (verb) {

		case blr_and:
		case blr_or:
			sql->append("(");
			if (!parseBlrBoolean(blr,end,req,sql,
						depth+1,gdscode)) {
				return false;
			}
			sql->append(op);
			if (!parseBlrBoolean(blr,end,req,sql,
						depth+1,gdscode)) {
				return false;
			}
			sql->append(")");
			break;

		case blr_not:
			sql->append("(NOT ");
			if (!parseBlrBoolean(blr,end,req,sql,
						depth+1,gdscode)) {
				return false;
			}
			sql->append(")");
			break;

		case blr_missing:
			sql->append("(");
			if (!parseBlrValue(blr,end,req,sql,
						NULL,NULL,gdscode)) {
				return false;
			}
			sql->append(" IS NULL)");
			break;

		case blr_between:
			sql->append("(");
			if (!parseBlrValue(blr,end,req,sql,
						NULL,NULL,gdscode)) {
				return false;
			}
			sql->append(" BETWEEN ");
			if (!parseBlrValue(blr,end,req,sql,
						NULL,NULL,gdscode)) {
				return false;
			}
			sql->append(" AND ");
			if (!parseBlrValue(blr,end,req,sql,
						NULL,NULL,gdscode)) {
				return false;
			}
			sql->append(")");
			break;

		case blr_eql:
		case blr_neq:
		case blr_gtr:
		case blr_geq:
		case blr_lss:
		case blr_leq:
		case blr_like:
		case blr_starting:
		case blr_containing:
			sql->append("(");
			if (!parseBlrValue(blr,end,req,sql,
						NULL,NULL,gdscode)) {
				return false;
			}
			sql->append(op);
			if (!parseBlrValue(blr,end,req,sql,
						NULL,NULL,gdscode)) {
				return false;
			}
			sql->append(")");
			break;

		default:
			if (getDebug()) {
				stdoutput.printf("	unsupported blr "
						"boolean verb: %u\n",verb);
			}
			*gdscode=isc_wish_list;
			return false;
	}

	return true;
}

bool sqlrprotocol_firebird::parseBlrValue(const byte_t **blr,
					const byte_t *end,
					sqlrfirebirdblrrequest *req,
					stringbuffer *sql,
					stringbuffer *raw,
					byte_t *kind,
					uint32_t *gdscode) {

	// value verbs:
	//
	// blr_field		<context> <name length> <name>
	// blr_literal		<data type> <type arguments> <value>
	// blr_parameter	<message number> <parameter number>
	// blr_parameter2	<message number> <parameter number>
	// 			<null indicator parameter number>
	// blr_null

	const byte_t	*p=*blr;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}

	byte_t	verb=*p;
	p++;

	if (kind) {
		*kind=verb;
	}

	switch (verb) {

		case blr_field:
			{
			if (p+2>end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			byte_t	context=*p;
			p++;
			byte_t	namelen=*p;
			p++;
			if (p+namelen>end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			if (context>=FIREBIRD_MAX_BLR_CONTEXTS ||
					!req->relations[context]) {
				*gdscode=isc_wish_list;
				return false;
			}
			sql->append("C")->append((uint32_t)context)->
								append(".");
			sql->append((const char *)p,(size_t)namelen);
			p+=namelen;
			}
			break;

		case blr_literal:
			*blr=p;
			if (!parseBlrLiteral(blr,end,sql,raw,gdscode)) {
				return false;
			}
			p=*blr;
			break;

		case blr_parameter:
		case blr_parameter2:
			{
			if (p+3>end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			byte_t	msgnumber=*p;
			p++;
			uint16_t	paramnumber=(uint16_t)(p[0]|(p[1]<<8));
			p+=2;
			if (verb==blr_parameter2) {
				if (p+2>end) {
					*gdscode=isc_dsql_error;
					return false;
				}
				p+=2;
			}
			// a parameter in an expression is a value the client
			// sends with op_start_send_and_receive, so the query
			// takes it as a bind and startRequest() fills it in
			if (!req->hasinmsg || msgnumber!=req->inmsg ||
				paramnumber>=req->msgfieldcount[msgnumber] ||
				req->inparamcount>=FIREBIRD_MAX_BLR_PARAMS) {
				*gdscode=isc_wish_list;
				return false;
			}
			req->inparams[req->inparamcount]=paramnumber;
			req->inparamcount++;
			sql->append("?");
			}
			break;

		case blr_null:
			sql->append("NULL");
			break;

		default:
			if (getDebug()) {
				stdoutput.printf("	unsupported blr "
						"value verb: %u\n",verb);
			}
			*gdscode=isc_wish_list;
			return false;
	}

	*blr=p;

	return true;
}

bool sqlrprotocol_firebird::parseBlrLiteral(const byte_t **blr,
					const byte_t *end,
					stringbuffer *sql,
					stringbuffer *raw,
					uint32_t *gdscode) {

	// blr_literal <data type> <type arguments> <value>
	//
	// The type arguments and the width of the value are the data type's
	// own - a scale byte and a fixed width integer, or a length and that
	// many characters.

	const byte_t	*p=*blr;

	if (p>=end) {
		*gdscode=isc_dsql_error;
		return false;
	}

	byte_t	dtype=*p;
	p++;

	switch (dtype) {

		case blr_short:
		case blr_long:
		case blr_int64:
			{
			if (p>=end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			int8_t	scale=(int8_t)*p;
			p++;
			uint16_t	width=(dtype==blr_short)?2:
					((dtype==blr_long)?4:8);
			if (p+width>end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			int64_t	val=0;
			for (uint16_t i=0; i<width; i++) {
				val|=((int64_t)p[i])<<(i*8);
			}
			// the value came off the wire in its own width, so
			// anything narrower than an int64 has to carry its
			// sign the rest of the way up
			if (width<8 &&
				(val&((int64_t)1<<(width*8-1)))) {
				val|=~(((int64_t)1<<(width*8))-1);
			}
			p+=width;
			// a scaled integer would have to be rendered against
			// the type it is going into, which the module doesn't
			// know here
			if (scale) {
				*gdscode=isc_wish_list;
				return false;
			}
			sql->append(val);
			if (raw) {
				raw->append(val);
			}
			}
			break;

		case blr_text:
		case blr_text2:
			{
			if (dtype==blr_text2) {
				// character set
				if (p+2>end) {
					*gdscode=isc_dsql_error;
					return false;
				}
				p+=2;
			}
			if (p+2>end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			uint16_t	len=(uint16_t)(p[0]|(p[1]<<8));
			p+=2;
			if (p+len>end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			sql->append("'");
			for (uint16_t i=0; i<len; i++) {
				// a quote inside the text would end the
				// string it sits in unless it is doubled
				if (p[i]=='\'') {
					sql->append('\'');
				}
				sql->append((char)p[i]);
			}
			sql->append("'");
			if (raw) {
				raw->append((const char *)p,(size_t)len);
			}
			p+=len;
			}
			break;

		case blr_bool:
			{
			if (p>=end) {
				*gdscode=isc_dsql_error;
				return false;
			}
			byte_t	val=*p;
			p++;
			sql->append((val)?"TRUE":"FALSE");
			if (raw) {
				raw->append((val)?"1":"0");
			}
			}
			break;

		default:
			if (getDebug()) {
				stdoutput.printf("	unsupported blr "
						"literal type: %u\n",dtype);
			}
			*gdscode=isc_wish_list;
			return false;
	}

	*blr=p;

	return true;
}

bool sqlrprotocol_firebird::buildBlrRequestQuery(
					sqlrfirebirdblrrequest *req,
					sqlrfirebirdblrwalk *walk,
					uint32_t *gdscode) {

	// a request that pulls from nothing, or sends nothing back, isn't
	// one there is a select to write for
	if (!walk->selectcount || !req->relationcount || !req->rowslots) {
		*gdscode=isc_wish_list;
		return false;
	}

	stringbuffer	query;
	query.append("SELECT ")->append(walk->selectclause.getString());
	query.append(" FROM ")->append(walk->fromclause.getString());
	if (walk->whereclause.getStringLength()) {
		query.append(" WHERE ")->
			append(walk->whereclause.getString());
	}
	if (walk->orderbyclause.getStringLength()) {
		query.append(" ORDER BY ")->
			append(walk->orderbyclause.getString());
	}
	if (walk->firstclause.getStringLength()) {
		query.append(" ROWS ")->
			append(walk->firstclause.getString());
	}

	uint32_t	querylen=(uint32_t)query.getStringLength();
	if (querylen>maxquerysize) {
		*gdscode=isc_dsql_error;
		return false;
	}

	req->query=charstring::duplicate(query.getString());
	req->querylen=querylen;

	return true;
}

bool sqlrprotocol_firebird::readMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *bytesread) {

	byte_t	*nullbits=NULL;
	if (!readMessageNullBits(fieldcount,&nullbits,bytesread)) {
		return false;
	}

	bool	retval=readMessageFields(cursor,fields,fieldcount,
						nullbits,bytesread);

	delete[] nullbits;

	return retval;
}

bool sqlrprotocol_firebird::readMessageNullBits(uint16_t fieldcount,
						byte_t **nullbits,
						uint32_t *bytesread) {

	// protocol 13 and up packs the message - the null indicators come
	// first, as one bit per field, and a field flagged null sends no
	// value at all.  before 13 each value is sent in full, whether it is
	// null or not, each one followed by its own 4-byte null indicator.
	*nullbits=NULL;
	if (protocolversion<PROTOCOL_VERSION13 || !fieldcount) {
		return true;
	}

	uint32_t	flagbytes=((uint32_t)fieldcount+7)/8;
	byte_t		*nb=new byte_t[flagbytes];
	bytestring::zero(nb,flagbytes);

	if (!readOpaque(nb,flagbytes,"null indicators",bytesread)) {
		delete[] nb;
		return false;
	}

	*nullbits=nb;

	return true;
}

bool sqlrprotocol_firebird::readMessageFields(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					const byte_t *nullbits,
					uint32_t *bytesread) {

	// everything the blr describes has to come off the socket, cursor or
	// no cursor, or the connection desynchronizes - only the binding is
	// conditional
	memorypool		*bindpool=NULL;
	sqlrserverbindvar	*inbinds=NULL;
	if (cursor) {
		bindpool=cont->getBindPool(cursor);
		inbinds=cont->getInputBinds(cursor);
	}

	uint16_t	bindcount=0;

	badblobid=false;

	for (uint16_t i=0; i<fieldcount; i++) {

		const sqlrfirebirdfield	*fld=&fields[i];

		// get the value
		sqlrfirebirdvalue	val;
		if (!readMessageValue(fld,nullbits,i,true,&val,bytesread)) {
			return false;
		}

		if (!inbinds || bindcount>=maxbindcount) {
			delete[] val.strval;
			continue;
		}

		bindMessageValue(bindpool,&(inbinds[bindcount]),
						bindcount,fld,&val);

		delete[] val.strval;

		bindcount++;
	}

	if (cursor) {
		cont->setInputBindCount(cursor,bindcount);
		if (getDebug()) {
			stdoutput.printf("	bound %u parameter(s)\n",
								bindcount);
		}
	}

	return true;
}

void sqlrprotocol_firebird::bindMessageValue(memorypool *bindpool,
					sqlrserverbindvar *bv,
					uint16_t bindindex,
					const sqlrfirebirdfield *fld,
					const sqlrfirebirdvalue *val) {

	bv->variable=bindvarnames[bindindex];
	bv->variablesize=bindvarnamesizes[bindindex];

	if (val->isnull) {
		bv->type=SQLRSERVERBINDVARTYPE_NULL;
		bv->isnull=cont->getNullBindValue();
	} else if (val->isquad &&
			getArrayById(val->blobhigh,val->bloblow)) {

		// an array parameter is bound as the bracketed,
		// comma-separated rendering the backend's own array bind
		// takes, since the backend's array id is its own and the
		// module's means nothing to it
		sqlrfirebirdarray	*array=
				getArrayById(val->blobhigh,val->bloblow);

		stringbuffer	elements;
		elements.append('{');
		bool		ok=true;
		const byte_t	*data=array->data.getBuffer();
		for (uint64_t i=0; i<array->elementcount && ok; i++) {
			if (i) {
				elements.append(',');
			}
			ok=appendArrayElement(&elements,
					array->elementtype,
					array->elementscale,
					data+i*array->elementsize,
					array->elementsize);
		}
		elements.append('}');

		if (ok) {
			uint64_t	size=elements.getStringLength();
			bv->type=SQLRSERVERBINDVARTYPE_ARRAY;
			bv->valuesize=(uint32_t)size;
			bv->value.stringval=
				(char *)bindpool->allocate(size+1);
			bytestring::copy(bv->value.stringval,
					elements.getString(),(size_t)size);
			bv->value.stringval[size]='\0';
			bv->isnull=cont->getNonNullBindValue();
		} else {
			// an element type with no text rendering - there is
			// nothing to bind for it
			bv->type=SQLRSERVERBINDVARTYPE_NULL;
			bv->isnull=cont->getNullBindValue();
		}

	} else if (val->isblob) {
		sqlrfirebirdblob	*blob=
				getBlobById(val->blobhigh,
						val->bloblow);
		if (!blob) {
			badblobid=true;
			bv->type=SQLRSERVERBINDVARTYPE_NULL;
			bv->isnull=cont->getNullBindValue();
		} else {
			// what gets bound is the bytes, since the
			// backend's blob id is its own and the
			// module's means nothing to it
			uint64_t	size=blob->data.getSize();
			if (size>0xffffffffULL) {
				size=0xffffffffULL;
			}
			bv->type=(fld->subtype==1)?
				SQLRSERVERBINDVARTYPE_CLOB:
				SQLRSERVERBINDVARTYPE_BLOB;
			bv->valuesize=(uint32_t)size;
			bv->value.stringval=
				(char *)bindpool->allocate(size+1);
			if (size) {
				bytestring::copy(bv->value.stringval,
						blob->data.getBuffer(),
						size);
			}
			bv->value.stringval[size]='\0';
			bv->isnull=cont->getNonNullBindValue();

			// hand the segment boundaries the client
			// wrote along with the bytes, so a backend
			// with its own notion of segments (eg.
			// firebird) can preserve them instead of
			// re-chunking the flat buffer
			uint32_t	segcount=blob->segcount;
			bv->segmentcount=(uint16_t)
				((segcount>0xffff)?0xffff:segcount);
			if (bv->segmentcount) {
				size_t	seglenbytes=
					(size_t)bv->segmentcount*
					sizeof(uint32_t);
				uint32_t	*seglens=(uint32_t *)
					bindpool->allocate(
						seglenbytes);
				bytestring::copy(seglens,
					blob->seglengths.getBuffer(),
					seglenbytes);
				bv->segmentlengths=seglens;
			}
		}
	} else if (val->strval) {
		bv->type=SQLRSERVERBINDVARTYPE_STRING;
		bv->valuesize=val->strvallen;
		bv->value.stringval=
			(char *)bindpool->allocate(val->strvallen+1);
		bytestring::copy(bv->value.stringval,
					val->strval,val->strvallen);
		bv->value.stringval[val->strvallen]='\0';
		bv->isnull=cont->getNonNullBindValue();
	} else if (val->isdate || val->istime) {
		bv->type=SQLRSERVERBINDVARTYPE_DATE;
		bv->value.dateval.year=1;
		bv->value.dateval.month=1;
		bv->value.dateval.day=1;
		bv->value.dateval.hour=0;
		bv->value.dateval.minute=0;
		bv->value.dateval.second=0;
		bv->value.dateval.microsecond=0;
		bv->value.dateval.tz=NULL;
		bv->value.dateval.isnegative=false;
		if (val->isdate) {
			decodeDate(val->dateval,
					&bv->value.dateval.year,
					&bv->value.dateval.month,
					&bv->value.dateval.day);
		}
		if (val->istime) {
			decodeTime(val->timeval,
					&bv->value.dateval.hour,
					&bv->value.dateval.minute,
					&bv->value.dateval.second,
					&bv->value.dateval.microsecond);
		}
		bv->isnull=cont->getNonNullBindValue();
	} else if (fld->blrtype==blr_float ||
			fld->blrtype==blr_double ||
			fld->blrtype==blr_d_float) {
		bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
		bv->value.doubleval.value=val->dblval;
		bv->value.doubleval.precision=0;
		bv->value.doubleval.scale=0;
		bv->isnull=cont->getNonNullBindValue();
	} else if (fld->scale) {
		// a scaled integer is a decimal whose point the wire
		// format leaves out
		bv->type=SQLRSERVERBINDVARTYPE_DOUBLE;
		double	divisor=1.0;
		for (int16_t s=fld->scale; s<0; s++) {
			divisor*=10.0;
		}
		bv->value.doubleval.value=(double)val->intval/divisor;
		bv->value.doubleval.precision=18;
		bv->value.doubleval.scale=-fld->scale;
		bv->isnull=cont->getNonNullBindValue();
	} else {
		bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
		bv->value.integerval=val->intval;
		bv->isnull=cont->getNonNullBindValue();
	}
}

bool sqlrprotocol_firebird::readMessageValue(const sqlrfirebirdfield *fld,
					const byte_t *nullbits,
					uint16_t index,
					bool nullindicator,
					sqlrfirebirdvalue *value,
					uint32_t *bytesread) {

	// what came off the wire, in whichever of these the type uses
	int64_t		intval=0;
	double		dblval=0.0;
	uint32_t	dateval=0;
	uint32_t	timeval=0;
	char		*strval=NULL;
	uint32_t	strvallen=0;
	bool		isdate=false;
	bool		istime=false;
	bool		isblob=false;
	bool		isquad=false;
	uint32_t	blobhigh=0;
	uint32_t	bloblow=0;

	// in the packed format the bitmap read up front is what says
	// whether the field is null, and a null field sends no value
	// at all - in the unpacked format the value always comes off
	// the wire and the indicator that follows it says
	bool	isnull=(nullbits && (nullbits[index>>3]&(1<<(index&7))));

	if (!isnull) {

		switch (fld->blrtype) {

			case blr_short:
			case blr_long:
				{
				uint32_t	val=0;
				if (!readInt(&val,"parameter",
							bytesread)) {
					return false;
				}
				intval=(int32_t)val;
				}
				break;

			case blr_int64:
				{
				uint64_t	val=0;
				if (!readInt64(&val,"parameter",
							bytesread)) {
					return false;
				}
				intval=(int64_t)val;
				}
				break;

			case blr_quad:
				// An array parameter is an id naming an
				// array the client built with
				// op_put_slice.  A blr_quad also turns up
				// for a blob parameter that an older
				// client described that way, though, and
				// the id itself is all there is to tell
				// the two apart - see ARRAY_ID_HIGH and
				// bindMessageValue().
				if (!readInt(&blobhigh,"parameter",
							bytesread) ||
					!readInt(&bloblow,"parameter",
							bytesread)) {
					return false;
				}
				isblob=true;
				isquad=true;
				break;

			case blr_blob2:
				// a blob parameter is an id naming a
				// blob the client built with
				// op_create_blob and filled with
				// op_put_segment
				if (!readInt(&blobhigh,"parameter",
							bytesread) ||
					!readInt(&bloblow,"parameter",
							bytesread)) {
					return false;
				}
				isblob=true;
				break;

			case blr_float:
				{
				uint32_t	val=0;
				if (!readInt(&val,"parameter",
							bytesread)) {
					return false;
				}
				float	f=0.0;
				bytestring::copy(&f,&val,sizeof(f));
				dblval=f;
				}
				break;

			case blr_double:
			case blr_d_float:
				{
				uint64_t	val=0;
				if (!readInt64(&val,"parameter",
							bytesread)) {
					return false;
				}
				bytestring::copy(&dblval,&val,
						sizeof(dblval));
				}
				break;

			case blr_sql_date:
				if (!readInt(&dateval,"parameter",
							bytesread)) {
					return false;
				}
				isdate=true;
				break;

			case blr_sql_time:
				if (!readInt(&timeval,"parameter",
							bytesread)) {
					return false;
				}
				istime=true;
				break;

			case blr_timestamp:
				if (!readInt(&dateval,"parameter",
							bytesread) ||
					!readInt(&timeval,"parameter",
							bytesread)) {
					return false;
				}
				isdate=true;
				istime=true;
				break;

			case blr_bool:
				{
				byte_t	val=0;
				if (!readOpaque(&val,1,"parameter",
							bytesread)) {
					return false;
				}
				intval=val;
				}
				break;

			case blr_text:
			case blr_text2:
				strvallen=fld->length;
				strval=new char[strvallen+1];
				if (!readOpaque((byte_t *)strval,
						strvallen,
						"parameter",
						bytesread)) {
					delete[] strval;
					return false;
				}
				strval[strvallen]='\0';
				break;

			case blr_cstring:
			case blr_cstring2:
				{
				// a cstring goes over length-first, like a
				// varying, rather than at its declared
				// width - see writeField()
				uint32_t	len=0;
				if (!readInt(&len,"parameter length",
							bytesread)) {
					return false;
				}
				// a length past the declared width would
				// leave the rest of the message misaligned,
				// so there's no reading on past it
				if (len>fld->length) {
					return false;
				}
				strvallen=len;
				strval=new char[strvallen+1];
				if (!readOpaque((byte_t *)strval,
						strvallen,
						"parameter",
						bytesread)) {
					delete[] strval;
					return false;
				}
				strval[strvallen]='\0';
				}
				break;

			case blr_varying:
			case blr_varying2:
				{
				uint32_t	len=0;
				if (!readInt(&len,"parameter length",
							bytesread)) {
					return false;
				}
				if (len>fld->length) {
					len=fld->length;
				}
				strvallen=len;
				strval=new char[strvallen+1];
				if (!readOpaque((byte_t *)strval,
						strvallen,
						"parameter",
						bytesread)) {
					delete[] strval;
					return false;
				}
				strval[strvallen]='\0';
				}
				break;

			default:
				// parseBlr() rejects anything else,
				// so this can't be reached
				return false;
		}
	}

	// null indicator
	if (!nullbits && nullindicator) {
		uint32_t	indicator=0;
		if (!readInt(&indicator,
				"null indicator",bytesread)) {
			delete[] strval;
			return false;
		}
		isnull=((int32_t)indicator<0);
	}

	// hand it all back
	// (the caller owns strval, and has to delete[] it)
	value->isnull=isnull;
	value->intval=intval;
	value->dblval=dblval;
	value->dateval=dateval;
	value->timeval=timeval;
	value->strval=strval;
	value->strvallen=strvallen;
	value->isdate=isdate;
	value->istime=istime;
	value->isblob=isblob;
	value->isquad=isquad;
	value->blobhigh=blobhigh;
	value->bloblow=bloblow;

	return true;
}

bool sqlrprotocol_firebird::writeMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *byteswritten) {

	uint32_t	colcount=cont->colCount(cursor);

	// protocol 13 and up packs the message - the null indicators go
	// first, as one bit per field, and a field flagged null sends no
	// value at all.  before 13 each value is sent in full, whether it is
	// null or not, each one followed by its own 4-byte null indicator.
	bool	packed=(protocolversion>=PROTOCOL_VERSION13);

	// null indicator bitmap
	if (packed && fieldcount) {

		uint32_t	flagbytes=((uint32_t)fieldcount+7)/8;
		byte_t		*nullbits=new byte_t[flagbytes];
		bytestring::zero(nullbits,flagbytes);

		for (uint16_t i=0; i<fieldcount; i++) {

			const char	*field=NULL;
			uint64_t	fieldsize=0;
			bool		lob=false;
			bool		null=true;

			if (i<colcount && !cont->getField(cursor,i,&field,
						&fieldsize,&lob,&null)) {
				delete[] nullbits;
				return false;
			}

			if (null) {
				nullbits[i>>3]|=(byte_t)(1<<(i&7));
			}
		}

		bool	retval=writeOpaque(nullbits,flagbytes,
					"null indicators",byteswritten);

		delete[] nullbits;

		if (!retval) {
			return false;
		}
	}

	for (uint16_t i=0; i<fieldcount; i++) {

		const char	*field=NULL;
		uint64_t	fieldsize=0;
		bool		lob=false;
		bool		null=true;

		if (i<colcount && !cont->getField(cursor,i,&field,
						&fieldsize,&lob,&null)) {
			return false;
		}

		// in the packed format the bitmap above already said the
		// field is null, and nothing at all goes on the wire for it
		if (packed && null) {
			continue;
		}

		if (!writeField(cursor,i,&fields[i],field,fieldsize,
					lob,null,byteswritten)) {
			return false;
		}

		// a null value still occupies its full width above - this is
		// what actually says it is null
		if (!packed && !writeInt((null)?0xffffffff:0,
					"null indicator",byteswritten)) {
			return false;
		}
	}

	return true;
}

bool sqlrprotocol_firebird::writeField(sqlrservercursor *cursor,
					uint32_t col,
					const sqlrfirebirdfield *fld,
					const char *value,
					uint64_t valuesize,
					bool lob,
					bool null,
					uint32_t *byteswritten) {

	const char	*v=(null || !value)?"":value;
	uint32_t	vlen=(null || !value)?0:(uint32_t)valuesize;

	switch (fld->blrtype) {

		case blr_short:
		case blr_long:
			return writeInt((uint32_t)(int32_t)
					scaledInteger(v,fld->scale),
					"field",byteswritten);

		case blr_int64:
			return writeInt64((uint64_t)
					scaledInteger(v,fld->scale),
					"field",byteswritten);

		case blr_float:
			{
			float		f=(float)charstring::convertToFloat(v);
			uint32_t	bits=0;
			bytestring::copy(&bits,&f,sizeof(bits));
			return writeInt(bits,"field",byteswritten);
			}

		case blr_double:
		case blr_d_float:
			{
			double		d=(double)charstring::convertToFloat(v);
			uint64_t	bits=0;
			bytestring::copy(&bits,&d,sizeof(bits));
			return writeInt64(bits,"field",byteswritten);
			}

		case blr_sql_date:
			{
			int32_t		parts[3]={1,1,1};
			splitNumbers(v,parts,3);
			return writeInt(encodeDate(parts[0],parts[1],parts[2]),
					"field",byteswritten);
			}

		case blr_sql_time:
			{
			int32_t		parts[4]={0,0,0,0};
			splitNumbers(v,parts,4);
			return writeInt(encodeTime(parts[0],parts[1],
							parts[2],parts[3]),
					"field",byteswritten);
			}

		case blr_timestamp:
			{
			int32_t		parts[7]={1,1,1,0,0,0,0};
			splitNumbers(v,parts,7);
			return writeInt(encodeDate(parts[0],parts[1],parts[2]),
					"field",byteswritten) &&
				writeInt(encodeTime(parts[3],parts[4],
							parts[5],parts[6]),
					"field",byteswritten);
			}

		case blr_bool:
			{
			byte_t	b=(!charstring::compare(v,"1") ||
					!charstring::compareIgnoringCase(
								v,"t") ||
					!charstring::compareIgnoringCase(
								v,"true"))?1:0;
			return writeOpaque(&b,1,"field",byteswritten);
			}

		case blr_quad:
		case blr_blob2:
			{
			// a fetched blob or array is answered with an id of
			// the module's own, since the backend's isn't
			// reachable through the server API
			// (the bytes are read here rather than when the
			// client asks for them, because the fetch loop has
			// moved on to the next row by then - see bufferBlob()
			// and bufferArray())
			uint32_t	high=0;
			uint32_t	low=0;
			if (!null) {
				if (cont->getColumnType(cursor,col)==
							ARRAY_DATATYPE) {
					bufferArray(cursor,col,&low);
					high=ARRAY_ID_HIGH;
				} else {
					bufferBlob(cursor,col,value,
							valuesize,lob,&low);
				}
			}
			return writeInt(high,"field",byteswritten) &&
				writeInt(low,"field",byteswritten);
			}

		case blr_text:
		case blr_text2:
			{
			// a fixed-width field is blank padded out to its
			// declared width
			byte_t	*buf=new byte_t[(fld->length)?fld->length:1];
			uint32_t	len=(vlen<fld->length)?
						vlen:fld->length;
			bytestring::copy(buf,v,len);
			if (len<fld->length) {
				bytestring::set(buf+len,' ',fld->length-len);
			}
			bool	retval=writeOpaque(buf,fld->length,
							"field",byteswritten);
			delete[] buf;
			return retval;
			}

		case blr_cstring:
		case blr_cstring2:
			{
			// A cstring is stored null-terminated but goes over
			// length-first, like a varying, and only the bytes
			// up to the terminator go at all.  The declared
			// width includes the terminator, so a value can only
			// be one byte shorter than it.
			uint32_t	width=fld->length;
			uint32_t	max=(width)?width-1:0;
			uint32_t	len=(vlen<max)?vlen:max;
			return writeInt(len,"field length",byteswritten) &&
				writeOpaque((const byte_t *)v,len,
						"field",byteswritten);
			}

		case blr_varying:
		case blr_varying2:
			{
			uint32_t	len=(vlen<fld->length)?
						vlen:fld->length;
			return writeInt(len,"field length",byteswritten) &&
				writeOpaque((const byte_t *)v,len,
						"field",byteswritten);
			}

		default:
			return false;
	}
}

uint16_t sqlrprotocol_firebird::sqlType(uint16_t coltype) {

	// (the low bit means the value carries a null indicator, so every
	// type answered here is even, and the describe turns the bit on)
	switch (coltype) {

		case SMALLINT_DATATYPE:
		case TINYINT_DATATYPE:
		case SHORT_DATATYPE:
		case USHORT_DATATYPE:
		case YEAR_DATATYPE:
		case BIT_DATATYPE:
		case BOOL_DATATYPE:
		case INT2_DATATYPE:
			return SQL_SHORT;

		case INT_DATATYPE:
		case INTEGER_DATATYPE:
		case MEDIUMINT_DATATYPE:
		case UINT_DATATYPE:
		case LONG_DATATYPE:
		case INT4_DATATYPE:
			return SQL_LONG;

		case BIGINT_DATATYPE:
		case LONGLONG_DATATYPE:
		case INT64_DATATYPE:
		case INT8_DATATYPE:
		case DECIMAL_DATATYPE:
		case NUMERIC_DATATYPE:
		case NUMBER_DATATYPE:
		case MONEY_DATATYPE:
		case SMALLMONEY_DATATYPE:
			return SQL_INT64;

		case FLOAT_DATATYPE:
		case REAL_DATATYPE:
		case FLOAT4_DATATYPE:
			return SQL_FLOAT;

		case DOUBLE_DATATYPE:
		case DOUBLE_PRECISION_DATATYPE:
		case FLOAT8_DATATYPE:
			return SQL_DOUBLE;

		case D_FLOAT_DATATYPE:
			return SQL_D_FLOAT;

		case DATE_DATATYPE:
		case NEWDATE_DATATYPE:
			return SQL_TYPE_DATE;

		case TIME_DATATYPE:
			return SQL_TYPE_TIME;

		case TIMESTAMP_DATATYPE:
		case DATETIME_DATATYPE:
		case SMALLDATETIME_DATATYPE:
			return SQL_TIMESTAMP;

		case CHAR_DATATYPE:
		case _CHAR_DATATYPE:
			return SQL_TEXT;

		// firebird has one lob type for everything, distinguished by
		// its sub type, so every character and binary lob maps onto it
		case BLOB_DATATYPE:
		case CLOB_DATATYPE:
		case DBCLOB_DATATYPE:
		case TINY_BLOB_DATATYPE:
		case MEDIUM_BLOB_DATATYPE:
		case LONG_BLOB_DATATYPE:
		case IMAGE_DATATYPE:
		case TEXT_DATATYPE:
		case LONGVARCHAR_DATATYPE:
		case LONGVARBINARY_DATATYPE:
		case LONGCHAR_DATATYPE:
		case LONGBINARY_DATATYPE:
		case LONG_RAW_DATATYPE:
		case BYTEA_DATATYPE:
			return SQL_BLOB;

		case ARRAY_DATATYPE:
			return SQL_ARRAY;

		case QUAD_DATATYPE:
			return SQL_QUAD;

		// varchar, and anything the module has no better answer for -
		// a string is the one thing every backend can render
		default:
			return SQL_VARYING;
	}
}

uint16_t sqlrprotocol_firebird::sqlLength(uint16_t sqltype,
						uint32_t colsize) {

	switch (sqltype) {
		case SQL_SHORT:
			return 2;
		case SQL_LONG:
		case SQL_FLOAT:
		case SQL_TYPE_DATE:
		case SQL_TYPE_TIME:
			return 4;
		case SQL_INT64:
		case SQL_DOUBLE:
		case SQL_D_FLOAT:
		case SQL_TIMESTAMP:
			return 8;
		case SQL_BLOB:
		case SQL_ARRAY:
		case SQL_QUAD:
			// the width of the id, not of the data
			return 8;
		case SQL_BOOLEAN:
			return 1;
		default:
			// text and varying, whose length is the column's
			return (colsize>MAX_CSTRING_LENGTH)?
					(uint16_t)MAX_CSTRING_LENGTH:
					(uint16_t)colsize;
	}
}

int16_t sqlrprotocol_firebird::sqlSubType(uint16_t coltype) {

	// for a number the sub type is what tells decimal from numeric -
	// firebird stores both as a scaled integer.  for text it is the
	// character set id, and for a lob 0 for binary and 1 for text.
	switch (coltype) {
		case NUMERIC_DATATYPE:
			return 1;
		case DECIMAL_DATATYPE:
		case MONEY_DATATYPE:
		case SMALLMONEY_DATATYPE:
			return 2;
		case CLOB_DATATYPE:
		case DBCLOB_DATATYPE:
		case TEXT_DATATYPE:
		case LONGVARCHAR_DATATYPE:
		case LONGCHAR_DATATYPE:
			return 1;
		default:
			return 0;
	}
}

bool sqlrprotocol_firebird::appendInfoBare(byte_t item) {

	// a bare item is one byte, with no length and no value.  the 1 held
	// back is for the trailing isc_info_end.
	if (respbuffer.getSize()+1>=respbufferlen) {
		if (respbuffer.getSize()<respbufferlen) {
			write(&respbuffer,(byte_t)isc_info_truncated);
		}
		if (getDebug()) {
			stdoutput.write("	truncated\n");
		}
		return false;
	}
	write(&respbuffer,item);
	return true;
}

bool sqlrprotocol_firebird::appendInfoRecords(sqlrservercursor *cursor,
						uint32_t stmttype) {

	uint32_t	affected=(cont->knowsAffectedRows(cursor))?
					(uint32_t)cont->getAffectedRows(cursor):0;

	uint32_t	counts[4]={0,0,0,0};
	switch (stmttype) {
		case isc_info_sql_stmt_insert:
			counts[3]=affected;
			break;
		case isc_info_sql_stmt_update:
			counts[0]=affected;
			break;
		case isc_info_sql_stmt_delete:
			counts[1]=affected;
			break;
		case isc_info_sql_stmt_select:
		case isc_info_sql_stmt_select_for_upd:
			counts[2]=(uint32_t)cont->getTotalRowsFetched(cursor);
			break;
		default:
			break;
	}

	// the value is an info buffer of its own, sub-clusters and a trailing
	// isc_info_end, in the order firebird itself writes them
	static const byte_t	items[4]={isc_info_req_update_count,
						isc_info_req_delete_count,
						isc_info_req_select_count,
						isc_info_req_insert_count};
	byte_t		val[29];
	uint16_t	vallen=0;
	for (uint8_t i=0; i<4; i++) {
		val[vallen++]=items[i];
		val[vallen++]=4;
		val[vallen++]=0;
		val[vallen++]=(byte_t)(counts[i]&0xff);
		val[vallen++]=(byte_t)((counts[i]>>8)&0xff);
		val[vallen++]=(byte_t)((counts[i]>>16)&0xff);
		val[vallen++]=(byte_t)((counts[i]>>24)&0xff);
	}
	val[vallen++]=isc_info_end;

	return appendInfoItem(isc_info_sql_records,val,vallen);
}

bool sqlrprotocol_firebird::appendInfoDescribe(sqlrservercursor *cursor,
						sqlrfirebirdstatement *stmt,
						bool bind,
						uint32_t start,
						const byte_t *items,
						uint32_t itemslen) {

	// when the backend has nothing prepared to describe, the shape
	// describeOutputColumns() probed for stands in for it - a real
	// describe, from a prepare or from an execute that already ran, always
	// wins
	bool	probed=!bind && !cont->colCount(cursor) &&
				stmt && stmt->probecols;

	// how many columns the result set has, or how many parameters the
	// query binds
	uint32_t	count=(bind)?
			cont->countBindVariables(cont->getQueryBuffer(cursor),
						cont->getQuerySize(cursor)):
			(probed)?stmt->probecolcount:
			cont->colCount(cursor);

	// the count is always the whole set, even when the groups below start
	// part way into it
	if (!appendInfoInt(isc_info_sql_describe_vars,count)) {
		return false;
	}

	// isc_info_sql_sqlda_start names the first column to describe, 1
	// based.  a client that had to ask twice asks the second time from
	// where the first reply left off.
	uint32_t	first=(start)?start-1:0;

	// SQL Relay carries no owner for a column, and the value is only ever
	// displayed, so the session's user stands in for it
	const char	*owner=(charstring::isNullOrEmpty(username))?
					"SYSDBA":username;

	for (uint32_t col=first; col<count; col++) {

		// the sequence has to lead each group - it says which column
		// the items after it describe, and a client that sees it late
		// rejects the whole reply
		if (!appendInfoInt(isc_info_sql_sqlda_seq,col+1)) {
			return false;
		}

		// describeBinds() works a bind's type out for an insert - a
		// bind whose type it couldn't work out describes as a nullable
		// string of a generous width, the one shape every backend can
		// convert from
		bool	described=bind && stmt && stmt->binds &&
					col<stmt->bindcount;
		uint16_t	coltype=UNKNOWN_DATATYPE;
		uint32_t	colsize=FIREBIRD_BIND_LENGTH;
		int32_t		colscale=0;
		const char	*colname="";
		const char	*coltable="";
		if (bind) {
			if (described) {
				coltype=stmt->binds[col].coltype;
				colsize=stmt->binds[col].colsize;
				colscale=-((int32_t)stmt->binds[col].colscale);
			}
		} else if (probed) {
			coltype=stmt->probecols[col].coltype;
			colsize=stmt->probecols[col].colsize;
			colscale=-((int32_t)stmt->probecols[col].colscale);
			colname=stmt->probecols[col].name;
			coltable=stmt->probecols[col].table;
		} else {
			coltype=cont->getColumnType(cursor,col);
			colsize=cont->getColumnSize(cursor,col);
			colscale=-((int32_t)cont->getColumnScale(cursor,col));
			colname=cont->getColumnName(cursor,col);
			coltable=cont->getColumnTable(cursor,col);
		}
		uint16_t	sqltype=sqlType(coltype);
		if (!colname) {
			colname="";
		}
		if (!coltable) {
			coltable="";
		}

		bool	fits=true;
		for (const byte_t *p=items; p<items+itemslen && fits; p++) {

			switch (*p) {

				case isc_info_sql_sqlda_seq:
				case isc_info_sql_describe_vars:
				case isc_info_sql_describe_end:
					// the group's own frame - written
					// around the loop rather than in it
					break;

				case isc_info_sql_type:
					// the low bit says the value carries a
					// null indicator, and every column the
					// module answers does
					fits=appendInfoInt(*p,sqltype+1);
					break;

				case isc_info_sql_sub_type:
					fits=appendInfoInt(*p,(uint32_t)
						(int32_t)sqlSubType(coltype));
					break;

				case isc_info_sql_scale:
					fits=appendInfoInt(*p,
							(uint32_t)colscale);
					break;

				case isc_info_sql_length:
					fits=appendInfoInt(*p,
						sqlLength(sqltype,colsize));
					break;

				case isc_info_sql_null_ind:
					fits=appendInfoInt(*p,1);
					break;

				case isc_info_sql_field:
				case isc_info_sql_alias:
					fits=appendInfoItem(*p,
						(const byte_t *)colname,
						charstring::getLength(colname));
					break;

				case isc_info_sql_relation:
				case isc_info_sql_relation_alias:
					fits=appendInfoItem(*p,
						(const byte_t *)coltable,
						charstring::getLength(
								coltable));
					break;

				case isc_info_sql_owner:
					fits=appendInfoItem(*p,
						(const byte_t *)owner,
						charstring::getLength(owner));
					break;

				default:
					fits=appendInfoError(*p);
					break;
			}
		}

		if (!fits || !appendInfoBare(isc_info_sql_describe_end)) {
			return false;
		}
	}

	return true;
}

bool sqlrprotocol_firebird::buildSqlInfo(sqlrservercursor *cursor,
					sqlrfirebirdstatement *stmt,
					const byte_t *items,
					uint32_t itemslen) {

	const byte_t	*p=items;
	const byte_t	*end=items+itemslen;

	uint32_t	stmttype=(stmt)?stmt->stmttype:0;

	// which column a describe starts at, until isc_info_sql_sqlda_start
	// says otherwise
	uint32_t	sqldastart=1;

	bool	fits=true;
	while (p<end && fits) {

		byte_t	item=*p;
		p++;

		switch (item) {

			case isc_info_end:
				p=end;
				break;

			case isc_info_sql_stmt_type:
				fits=appendInfoInt(item,stmttype);
				break;

			case isc_info_sql_stmt_flags:
				{
				uint32_t	flags=FB_STMT_REPEAT_EXECUTE;
				if (stmttype==isc_info_sql_stmt_select ||
					stmttype==
					isc_info_sql_stmt_select_for_upd) {
					flags|=FB_STMT_HAS_CURSOR;
				}
				fits=appendInfoInt(item,flags);
				}
				break;

			case isc_info_sql_records:
				fits=appendInfoRecords(cursor,stmttype);
				break;

			case isc_info_sql_bind:
			case isc_info_sql_select:
				{
				// everything up to isc_info_sql_describe_end
				// is the template each column's group repeats
				const byte_t	*tmpl=p;
				while (p<end &&
					*p!=isc_info_sql_describe_end) {
					// isc_info_sql_sqlda_start carries a
					// one-byte inline length - the only
					// item in the protocol that does, and
					// skipping it as a bare byte
					// desynchronizes the walk
					if (*p==isc_info_sql_sqlda_start &&
								p+1<end) {
						uint32_t	skip=2+p[1];
						p=(p+skip>end)?end:p+skip;
						continue;
					}
					p++;
				}
				uint32_t	tmpllen=p-tmpl;
				if (p<end) {
					p++;
				}
				fits=appendInfoBare(item) &&
					appendInfoDescribe(cursor,stmt,
						item==isc_info_sql_bind,
						sqldastart,tmpl,tmpllen);
				}
				break;

			case isc_info_sql_sqlda_start:
				{
				// a one-byte inline length, then a
				// little-endian value - the only item in the
				// protocol shaped this way
				if (p>=end) {
					break;
				}
				byte_t	len=*p;
				p++;
				sqldastart=0;
				for (byte_t i=0; i<len && p<end; i++) {
					// anything past the fourth byte would
					// shift a 32 bit value off its own end
					if (i<sizeof(uint32_t)) {
						sqldastart|=
							((uint32_t)*p)<<(8*i);
					}
					p++;
				}
				}
				break;

			default:
				fits=appendInfoError(item);
				break;
		}
	}

	// a reply that wasn't truncated ends with a bare isc_info_end
	if (fits && respbuffer.getSize()<respbufferlen) {
		write(&respbuffer,(byte_t)isc_info_end);
	}

	return fits;
}

int64_t sqlrprotocol_firebird::scaledInteger(const char *value,
						int16_t scale) {

	// a scaled number goes on the wire as an integer with the decimal
	// point left out - 1.5 at scale -2 is 150 - so the digits have to be
	// shifted rather than the value divided, or the last digit rounds away
	if (charstring::isNullOrEmpty(value)) {
		return 0;
	}

	const char	*p=value;
	while (character::isWhitespace(*p)) {
		p++;
	}

	bool	negative=false;
	if (*p=='-') {
		negative=true;
		p++;
	} else if (*p=='+') {
		p++;
	}

	int64_t	result=0;
	while (character::isDigit(*p)) {
		result=result*10+(*p-'0');
		p++;
	}

	int32_t	digits=(scale<0)?-scale:0;
	if (*p=='.') {
		p++;
		while (digits && character::isDigit(*p)) {
			result=result*10+(*p-'0');
			p++;
			digits--;
		}
	}
	while (digits) {
		result*=10;
		digits--;
	}

	// a positive scale means the value is stored shifted the other way
	for (int16_t i=0; i<scale; i++) {
		result/=10;
	}

	return (negative)?-result:result;
}

uint16_t sqlrprotocol_firebird::splitNumbers(const char *value,
						int32_t *parts,
						uint16_t maxparts) {

	// the firebird connection module renders a date as yyyy:mm:dd, a time
	// as hh:mm:ss and a timestamp as "yyyy-mm-dd hh:mm:ss", so what
	// separates the numbers varies but their order never does
	// (a delimiter-driven parse would be ambiguous on a colon-delimited
	// date)
	uint16_t	count=0;
	const char	*p=value;
	while (p && *p && count<maxparts) {
		if (!character::isDigit(*p)) {
			p++;
			continue;
		}
		int32_t	val=0;
		while (character::isDigit(*p)) {
			val=val*10+(*p-'0');
			p++;
		}
		parts[count]=val;
		count++;
	}
	return count;
}

uint32_t sqlrprotocol_firebird::encodeDate(int32_t year,
						int32_t month,
						int32_t day) {

	// firebird counts days from 17 November 1858, the modified julian day
	// epoch.  this is its own encode_date, digit for digit.
	int32_t	y=year;
	int32_t	m=month;
	if (m>2) {
		m-=3;
	} else {
		m+=9;
		y--;
	}
	int32_t	c=y/100;
	int32_t	ya=y-100*c;
	return (uint32_t)((146097*c)/4+(1461*ya)/4+
				(153*m+2)/5+day+1721119-2400001);
}

uint32_t sqlrprotocol_firebird::encodeTime(int32_t hour,
						int32_t minute,
						int32_t second,
						int32_t fraction) {

	// an ISC_TIME counts ten-thousandths of a second since midnight
	return (uint32_t)((((hour*60)+minute)*60+second)*
				FIREBIRD_TIME_PRECISION+fraction);
}

void sqlrprotocol_firebird::decodeDate(uint32_t date,
					int16_t *year,
					int16_t *month,
					int16_t *day) {

	// firebird's own decode_date, digit for digit.  The day count comes
	// off the wire, and the arithmetic below multiplies it by four, so a
	// value near the top of the range has to be refused rather than
	// overflowed into a garbage date.
	if (date>MAX_FIREBIRD_DATE) {
		*year=1;
		*month=1;
		*day=1;
		return;
	}
	int32_t	nday=(int32_t)date+2400001-1721119;
	int32_t	century=(4*nday-1)/146097;
	nday=4*nday-1-146097*century;
	int32_t	d=nday/4;
	nday=(4*d+3)/1461;
	d=4*d+3-1461*nday;
	d=(d+4)/4;
	int32_t	m=(5*d-3)/153;
	d=5*d-3-153*m;
	d=(d+5)/5;
	int32_t	y=100*century+nday;
	if (m<10) {
		m+=3;
	} else {
		m-=9;
		y++;
	}
	*year=(int16_t)y;
	*month=(int16_t)m;
	*day=(int16_t)d;
}

void sqlrprotocol_firebird::decodeTime(uint32_t time,
					int16_t *hour,
					int16_t *minute,
					int16_t *second,
					int32_t *microsecond) {

	uint32_t	t=time;
	*hour=(int16_t)(t/(FIREBIRD_TIME_PRECISION*3600));
	t%=(FIREBIRD_TIME_PRECISION*3600);
	*minute=(int16_t)(t/(FIREBIRD_TIME_PRECISION*60));
	t%=(FIREBIRD_TIME_PRECISION*60);
	*second=(int16_t)(t/FIREBIRD_TIME_PRECISION);
	*microsecond=(int32_t)((t%FIREBIRD_TIME_PRECISION)*100);
}

void sqlrprotocol_firebird::debugSystemError() {
	if (!getDebug()) {
		return;
	}
	char	*err=error::getErrorString();
	stdoutput.printf("%s\n",err);
	delete[] err;
}

void sqlrprotocol_firebird::debugOpCode(const char *name, uint32_t opcode) {
	if (!getDebug()) {
		return;
	}
	const char	*opcodestr=NULL;
	switch (opcode) {
		case op_void:
			opcodestr="op_void";
			break;
		case op_connect:
			opcodestr="op_connect";
			break;
		case op_exit:
			opcodestr="op_exit";
			break;
		case op_accept:
			opcodestr="op_accept";
			break;
		case op_reject:
			opcodestr="op_reject";
			break;
		case op_disconnect:
			opcodestr="op_disconnect";
			break;
		case op_response:
			opcodestr="op_response";
			break;
		case op_attach:
			opcodestr="op_attach";
			break;
		case op_create:
			opcodestr="op_create";
			break;
		case op_detach:
			opcodestr="op_detach";
			break;
		case op_compile:
			opcodestr="op_compile";
			break;
		case op_start:
			opcodestr="op_start";
			break;
		case op_start_and_send:
			opcodestr="op_start_and_send";
			break;
		case op_send:
			opcodestr="op_send";
			break;
		case op_receive:
			opcodestr="op_receive";
			break;
		case op_unwind:
			opcodestr="op_unwind";
			break;
		case op_release:
			opcodestr="op_release";
			break;
		case op_transaction:
			opcodestr="op_transaction";
			break;
		case op_commit:
			opcodestr="op_commit";
			break;
		case op_rollback:
			opcodestr="op_rollback";
			break;
		case op_prepare:
			opcodestr="op_prepare";
			break;
		case op_reconnect:
			opcodestr="op_reconnect";
			break;
		case op_create_blob:
			opcodestr="op_create_blob";
			break;
		case op_open_blob:
			opcodestr="op_open_blob";
			break;
		case op_get_segment:
			opcodestr="op_get_segment";
			break;
		case op_put_segment:
			opcodestr="op_put_segment";
			break;
		case op_cancel_blob:
			opcodestr="op_cancel_blob";
			break;
		case op_close_blob:
			opcodestr="op_close_blob";
			break;
		case op_info_database:
			opcodestr="op_info_database";
			break;
		case op_info_request:
			opcodestr="op_info_request";
			break;
		case op_info_transaction:
			opcodestr="op_info_transaction";
			break;
		case op_info_blob:
			opcodestr="op_info_blob";
			break;
		case op_batch_segments:
			opcodestr="op_batch_segments";
			break;
		case op_que_events:
			opcodestr="op_que_events";
			break;
		case op_cancel_events:
			opcodestr="op_cancel_events";
			break;
		case op_commit_retaining:
			opcodestr="op_commit_retaining";
			break;
		case op_prepare2:
			opcodestr="op_prepare2";
			break;
		case op_event:
			opcodestr="op_event";
			break;
		case op_connect_request:
			opcodestr="op_connect_request";
			break;
		case op_aux_connect:
			opcodestr="op_aux_connect";
			break;
		case op_ddl:
			opcodestr="op_ddl";
			break;
		case op_open_blob2:
			opcodestr="op_open_blob2";
			break;
		case op_create_blob2:
			opcodestr="op_create_blob2";
			break;
		case op_get_slice:
			opcodestr="op_get_slice";
			break;
		case op_put_slice:
			opcodestr="op_put_slice";
			break;
		case op_slice:
			opcodestr="op_slice";
			break;
		case op_seek_blob:
			opcodestr="op_seek_blob";
			break;
		case op_allocate_statement:
			opcodestr="op_allocate_statement";
			break;
		case op_execute:
			opcodestr="op_execute";
			break;
		case op_exec_immediate:
			opcodestr="op_exec_immediate";
			break;
		case op_fetch:
			opcodestr="op_fetch";
			break;
		case op_fetch_response:
			opcodestr="op_fetch_response";
			break;
		case op_free_statement:
			opcodestr="op_free_statement";
			break;
		case op_prepare_statement:
			opcodestr="op_prepare_statement";
			break;
		case op_set_cursor:
			opcodestr="op_set_cursor";
			break;
		case op_info_sql:
			opcodestr="op_info_sql";
			break;
		case op_dummy:
			opcodestr="op_dummy";
			break;
		case op_response_piggyback:
			opcodestr="op_response_piggyback";
			break;
		case op_start_and_receive:
			opcodestr="op_start_and_receive";
			break;
		case op_start_send_and_receive:
			opcodestr="op_start_send_and_receive";
			break;
		case op_exec_immediate2:
			opcodestr="op_exec_immediate2";
			break;
		case op_execute2:
			opcodestr="op_execute2";
			break;
		case op_insert:
			opcodestr="op_insert";
			break;
		case op_sql_response:
			opcodestr="op_sql_response";
			break;
		case op_transact:
			opcodestr="op_transact";
			break;
		case op_transact_response:
			opcodestr="op_transact_response";
			break;
		case op_drop_database:
			opcodestr="op_drop_database";
			break;
		case op_service_attach:
			opcodestr="op_service_attach";
			break;
		case op_service_detach:
			opcodestr="op_service_detach";
			break;
		case op_service_info:
			opcodestr="op_service_info";
			break;
		case op_service_start:
			opcodestr="op_service_start";
			break;
		case op_rollback_retaining:
			opcodestr="op_rollback_retaining";
			break;
		case op_update_account_info:
			opcodestr="op_update_account_info";
			break;
		case op_authenticate_user:
			opcodestr="op_authenticate_user";
			break;
		case op_partial:
			opcodestr="op_partial";
			break;
		case op_trusted_auth:
			opcodestr="op_trusted_auth";
			break;
		case op_cancel:
			opcodestr="op_cancel";
			break;
		case op_cont_auth:
			opcodestr="op_cont_auth";
			break;
		case op_ping:
			opcodestr="op_ping";
			break;
		case op_accept_data:
			opcodestr="op_accept_data";
			break;
		case op_abort_aux_connection:
			opcodestr="op_abort_aux_connection";
			break;
		case op_crypt:
			opcodestr="op_crypt";
			break;
		case op_crypt_key_callback:
			opcodestr="op_crypt_key_callback";
			break;
		case op_cond_accept:
			opcodestr="op_cond_accept";
			break;
		case op_batch_create:
			opcodestr="op_batch_create";
			break;
		case op_batch_msg:
			opcodestr="op_batch_msg";
			break;
		case op_batch_exec:
			opcodestr="op_batch_exec";
			break;
		case op_batch_rls:
			opcodestr="op_batch_rls";
			break;
		case op_batch_cs:
			opcodestr="op_batch_cs";
			break;
		case op_batch_regblob:
			opcodestr="op_batch_regblob";
			break;
		case op_batch_blob_stream:
			opcodestr="op_batch_blob_stream";
			break;
		case op_batch_set_bpb:
			opcodestr="op_batch_set_bpb";
			break;
		case op_repl_data:
			opcodestr="op_repl_data";
			break;
		case op_repl_req:
			opcodestr="op_repl_req";
			break;
		case op_batch_cancel:
			opcodestr="op_batch_cancel";
			break;
		case op_batch_sync:
			opcodestr="op_batch_sync";
			break;
		case op_info_batch:
			opcodestr="op_info_batch";
			break;
		case op_fetch_scroll:
			opcodestr="op_fetch_scroll";
			break;
		case op_info_cursor:
			opcodestr="op_info_cursor";
			break;
		case op_inline_blob:
			opcodestr="op_inline_blob";
			break;
		default:
			opcodestr="unknown";
			break;
	}
	stdoutput.printf("	%s: 0x%02x %s\n",name,opcode,opcodestr);
}

void sqlrprotocol_firebird::debugArchType(uint32_t archtype) {
	if (!getDebug()) {
		return;
	}
	const char	*archtypestr=NULL;
	switch (archtype) {
		case arch_generic:
			archtypestr="arch_generic";
			break;
		case arch_apollo:
			archtypestr="arch_apollo";
			break;
		case arch_sun:
			archtypestr="arch_sun";
			break;
		case arch_vms:
			archtypestr="arch_vms";
			break;
		case arch_ultrix:
			archtypestr="arch_ultrix";
			break;
		case arch_alliant:
			archtypestr="arch_alliant";
			break;
		case arch_msdos:
			archtypestr="arch_msdos";
			break;
		case arch_sun4:
			archtypestr="arch_sun4";
			break;
		case arch_sunx86:
			archtypestr="arch_sunx86";
			break;
		case arch_hpux:
			archtypestr="arch_hpux";
			break;
		case arch_hpmpexl:
			archtypestr="arch_hpmpexl";
			break;
		case arch_mac:
			archtypestr="arch_mac";
			break;
		case arch_macaux:
			archtypestr="arch_macaux";
			break;
		case arch_rt:
			archtypestr="arch_rt";
			break;
		case arch_mips_ultrix:
			archtypestr="arch_mips_ultrix";
			break;
		case arch_hpux_68k:
			archtypestr="arch_hpux_68k";
			break;
		case arch_xenix:
			archtypestr="arch_xenix";
			break;
		case arch_aviion:
			archtypestr="arch_aviion";
			break;
		case arch_sgi:
			archtypestr="arch_sgi";
			break;
		case arch_apollo_dn10k:
			archtypestr="arch_apollo_dn10k";
			break;
		case arch_cray:
			archtypestr="arch_cray";
			break;
		case arch_imp:
			archtypestr="arch_imp";
			break;
		case arch_delta:
			archtypestr="arch_delta";
			break;
		case arch_sco:
			archtypestr="arch_sco";
			break;
		case arch_next:
			archtypestr="arch_next";
			break;
		case arch_next_386:
			archtypestr="arch_next_386";
			break;
		case arch_m88k:
			archtypestr="arch_m88k";
			break;
		case arch_unixware:
			archtypestr="arch_unixware";
			break;
		case arch_intel_32:
			archtypestr="arch_intel_32";
			break;
		case arch_epson:
			archtypestr="arch_epson";
			break;
		case arch_decosf:
			archtypestr="arch_decosf";
			break;
		case arch_ncr3000:
			archtypestr="arch_ncr3000";
			break;
		case arch_nt_ppc:
			archtypestr="arch_nt_ppc";
			break;
		case arch_dg_x86:
			archtypestr="arch_dg_x86";
			break;
		case arch_sco_ev:
			archtypestr="arch_sco_ev";
			break;
		case arch_linux:
			archtypestr="arch_linux";
			break;
		case arch_freebsd:
			archtypestr="arch_freebsd";
			break;
		case arch_netbsd:
			archtypestr="arch_netbsd";
			break;
		case arch_darwin_ppc:
			archtypestr="arch_darwin_ppc";
			break;
		case arch_winnt_64:
			archtypestr="arch_winnt_64";
			break;
		case arch_darwin_x64:
			archtypestr="arch_darwin_x64";
			break;
		case arch_darwin_ppc64:
			archtypestr="arch_darwin_ppc64";
			break;
		case arch_arm:
			archtypestr="arch_arm";
			break;
		case arch_winnt_arm64:
			archtypestr="arch_winnt_arm64";
			break;
		default:
			archtypestr="unknown";
			break;
	}
	stdoutput.printf("	arch type: %s\n",archtypestr);
}

void sqlrprotocol_firebird::debugProtocolVersion(uint32_t protoversion) {
	if (!getDebug()) {
		return;
	}
	const char	*protoversionstr=NULL;
	switch (protoversion) {
		case PROTOCOL_VERSION3:
			protoversionstr="PROTOCOL_VERSION3";
			break;
		case PROTOCOL_VERSION4:
			protoversionstr="PROTOCOL_VERSION4";
			break;
		case PROTOCOL_VERSION5:
			protoversionstr="PROTOCOL_VERSION5";
			break;
		case PROTOCOL_VERSION6:
			protoversionstr="PROTOCOL_VERSION6";
			break;
		case PROTOCOL_VERSION7:
			protoversionstr="PROTOCOL_VERSION7";
			break;
		case PROTOCOL_VERSION8:
			protoversionstr="PROTOCOL_VERSION8";
			break;
		case PROTOCOL_VERSION9:
			protoversionstr="PROTOCOL_VERSION9";
			break;
		case PROTOCOL_VERSION10:
			protoversionstr="PROTOCOL_VERSION10";
			break;
		case PROTOCOL_VERSION11:
			protoversionstr="PROTOCOL_VERSION11";
			break;
		case PROTOCOL_VERSION12:
			protoversionstr="PROTOCOL_VERSION12";
			break;
		case PROTOCOL_VERSION13:
			protoversionstr="PROTOCOL_VERSION13";
			break;
		case PROTOCOL_VERSION14:
			protoversionstr="PROTOCOL_VERSION14";
			break;
		case PROTOCOL_VERSION15:
			protoversionstr="PROTOCOL_VERSION15";
			break;
		case PROTOCOL_VERSION16:
			protoversionstr="PROTOCOL_VERSION16";
			break;
		case PROTOCOL_VERSION17:
			protoversionstr="PROTOCOL_VERSION17";
			break;
		case PROTOCOL_VERSION18:
			protoversionstr="PROTOCOL_VERSION18";
			break;
		case PROTOCOL_VERSION19:
			protoversionstr="PROTOCOL_VERSION19";
			break;
		case PROTOCOL_VERSION20:
			protoversionstr="PROTOCOL_VERSION20";
			break;
		default:
			protoversionstr="unknown";
			break;
	}
	stdoutput.printf("	protocol version: 0x%04x %s\n",
				protoversion,protoversionstr);
}

void sqlrprotocol_firebird::debugProtocolType(const char *title,
						uint32_t protocoltype) {
	if (!getDebug()) {
		return;
	}
	// the type is the low byte, the flags are the upper byte
	const char	*protocoltypestr=NULL;
	switch (protocoltype&ptype_MASK) {
		case ptype_page:
			protocoltypestr="ptype_page";
			break;
		case ptype_rpc:
			protocoltypestr="ptype_rpc";
			break;
		case ptype_batch_send:
			protocoltypestr="ptype_batch_send";
			break;
		case ptype_out_of_band:
			protocoltypestr="ptype_out_of_band";
			break;
		case ptype_lazy_send:
			protocoltypestr="ptype_lazy_send";
			break;
		default:
			protocoltypestr="unknown";
			break;
	}
	stdoutput.printf("	%s: %s\n",
				title,protocoltypestr);

	// flags
	if (protocoltype&pflag_compress) {
		stdoutput.write("		pflag_compress\n");
	}
	if (protocoltype&pflag_win_sspi_nego) {
		stdoutput.write("		pflag_win_sspi_nego\n");
	}
}

void sqlrprotocol_firebird::debugConnectVersion(uint32_t connectversion) {
	if (!getDebug()) {
		return;
	}
	const char	*connectversionstr=NULL;
	switch (connectversion) {
		case CONNECT_VERSION2:
			connectversionstr="CONNECT_VERSION2";
			break;
		case CONNECT_VERSION3:
			connectversionstr="CONNECT_VERSION3";
			break;
		default:
			connectversionstr="unknown";
			break;
	}
	stdoutput.printf("	connect version: %u (%s)\n",
				connectversion,connectversionstr);
}

void sqlrprotocol_firebird::debugUserId(const byte_t *userid,
						uint32_t useridlen) {
	if (!getDebug()) {
		return;
	}

	// get each tag...
	// (every tag is a type byte, a 1-byte value length, and that many
	// value bytes)
	const byte_t	*ptr=userid;
	const byte_t	*endptr=userid+useridlen;
	while ((size_t)(endptr-ptr)>1) {

		// get the type
		byte_t	tag=0;
		read(ptr,&tag,&ptr);

		const char	*tagstr=NULL;
		switch (tag) {
			case CNCT_user:
				tagstr="CNCT_user";
				break;
			case CNCT_passwd:
				tagstr="CNCT_passwd";
				break;
			case CNCT_ppo:
				tagstr="CNCT_ppo";
				break;
			case CNCT_host:
				tagstr="CNCT_host";
				break;
			case CNCT_group:
				tagstr="CNCT_group";
				break;
			case CNCT_user_verification:
				tagstr="CNCT_user_verification";
				break;
			case CNCT_specific_data:
				tagstr="CNCT_specific_data";
				break;
			case CNCT_plugin_name:
				tagstr="CNCT_plugin_name";
				break;
			case CNCT_login:
				tagstr="CNCT_login";
				break;
			case CNCT_plugin_list:
				tagstr="CNCT_plugin_list";
				break;
			case CNCT_client_crypt:
				tagstr="CNCT_client_crypt";
				break;
			default:
				tagstr="unknown";
				break;
		}
		stdoutput.printf("	user id tag: %u (0x%02x) (%s)\n",
							tag,tag,tagstr);

		// get the value length
		byte_t	valuelen=0;
		read(ptr,&valuelen,&ptr);
		stdoutput.printf("	user id value length: %u\n",valuelen);

		// bail if the value runs past the end of the buffer
		if ((size_t)valuelen>(size_t)(endptr-ptr)) {
			stdoutput.write("	user id value runs past "
					"the end of the buffer\n");
			break;
		}

		// step over the value
		if (valuelen) {
			stdoutput.printHex(ptr,valuelen);
		}
		ptr+=valuelen;
	}
}

void sqlrprotocol_firebird::debugDpbVersion(byte_t dpbversion) {
	if (!getDebug()) {
		return;
	}
	const char	*dpbversionstr=NULL;
	switch (dpbversion) {
		case isc_dpb_version1:
			dpbversionstr="isc_dpb_version1";
			break;
		case isc_dpb_version2:
			dpbversionstr="isc_dpb_version2";
			break;
		default:
			dpbversionstr="unknown";
			break;
	}
	stdoutput.printf("	dpb version: %u (%s)\n",
				dpbversion,dpbversionstr);
}

void sqlrprotocol_firebird::debugDpbParam(byte_t dpbparam) {
	if (!getDebug()) {
		return;
	}
	const char	*dpbparamstr=NULL;
	switch (dpbparam) {
		case isc_dpb_cdd_pathname:
			dpbparamstr="isc_dpb_cdd_pathname";
			break;
		case isc_dpb_allocation:
			dpbparamstr="isc_dpb_allocation";
			break;
		case isc_dpb_journal:
			dpbparamstr="isc_dpb_journal";
			break;
		case isc_dpb_page_size:
			dpbparamstr="isc_dpb_page_size";
			break;
		case isc_dpb_num_buffers:
			dpbparamstr="isc_dpb_num_buffers";
			break;
		case isc_dpb_buffer_length:
			dpbparamstr="isc_dpb_buffer_length";
			break;
		case isc_dpb_debug:
			dpbparamstr="isc_dpb_debug";
			break;
		case isc_dpb_garbage_collect:
			dpbparamstr="isc_dpb_garbage_collect";
			break;
		case isc_dpb_verify:
			dpbparamstr="isc_dpb_verify";
			break;
		case isc_dpb_sweep:
			dpbparamstr="isc_dpb_sweep";
			break;
		case isc_dpb_enable_journal:
			dpbparamstr="isc_dpb_enable_journal";
			break;
		case isc_dpb_disable_journal:
			dpbparamstr="isc_dpb_disable_journal";
			break;
		case isc_dpb_dbkey_scope:
			dpbparamstr="isc_dpb_dbkey_scope";
			break;
		case isc_dpb_number_of_users:
			dpbparamstr="isc_dpb_number_of_users";
			break;
		case isc_dpb_trace:
			dpbparamstr="isc_dpb_trace";
			break;
		case isc_dpb_no_garbage_collect:
			dpbparamstr="isc_dpb_no_garbage_collect";
			break;
		case isc_dpb_damaged:
			dpbparamstr="isc_dpb_damaged";
			break;
		case isc_dpb_license:
			dpbparamstr="isc_dpb_license";
			break;
		case isc_dpb_sys_user_name:
			dpbparamstr="isc_dpb_sys_user_name";
			break;
		case isc_dpb_encrypt_key:
			dpbparamstr="isc_dpb_encrypt_key";
			break;
		case isc_dpb_activate_shadow:
			dpbparamstr="isc_dpb_activate_shadow";
			break;
		case isc_dpb_sweep_interval:
			dpbparamstr="isc_dpb_sweep_interval";
			break;
		case isc_dpb_delete_shadow:
			dpbparamstr="isc_dpb_delete_shadow";
			break;
		case isc_dpb_force_write:
			dpbparamstr="isc_dpb_force_write";
			break;
		case isc_dpb_begin_log:
			dpbparamstr="isc_dpb_begin_log";
			break;
		case isc_dpb_quit_log:
			dpbparamstr="isc_dpb_quit_log";
			break;
		case isc_dpb_no_reserve:
			dpbparamstr="isc_dpb_no_reserve";
			break;
		case isc_dpb_user_name:
			dpbparamstr="isc_dpb_user_name";
			break;
		case isc_dpb_password:
			dpbparamstr="isc_dpb_password";
			break;
		case isc_dpb_password_enc:
			dpbparamstr="isc_dpb_password_enc";
			break;
		case isc_dpb_sys_user_name_enc:
			dpbparamstr="isc_dpb_sys_user_name_enc";
			break;
		case isc_dpb_interp:
			dpbparamstr="isc_dpb_interp";
			break;
		case isc_dpb_online_dump:
			dpbparamstr="isc_dpb_online_dump";
			break;
		case isc_dpb_old_file_size:
			dpbparamstr="isc_dpb_old_file_size";
			break;
		case isc_dpb_old_num_files:
			dpbparamstr="isc_dpb_old_num_files";
			break;
		case isc_dpb_old_file:
			dpbparamstr="isc_dpb_old_file";
			break;
		case isc_dpb_old_start_page:
			dpbparamstr="isc_dpb_old_start_page";
			break;
		case isc_dpb_old_start_seqno:
			dpbparamstr="isc_dpb_old_start_seqno";
			break;
		case isc_dpb_old_start_file:
			dpbparamstr="isc_dpb_old_start_file";
			break;
		case isc_dpb_drop_walfile:
			dpbparamstr="isc_dpb_drop_walfile";
			break;
		case isc_dpb_old_dump_id:
			dpbparamstr="isc_dpb_old_dump_id";
			break;
		case isc_dpb_wal_backup_dir:
			dpbparamstr="isc_dpb_wal_backup_dir";
			break;
		case isc_dpb_wal_chkptlen:
			dpbparamstr="isc_dpb_wal_chkptlen";
			break;
		case isc_dpb_wal_numbufs:
			dpbparamstr="isc_dpb_wal_numbufs";
			break;
		case isc_dpb_wal_bufsize:
			dpbparamstr="isc_dpb_wal_bufsize";
			break;
		case isc_dpb_wal_grp_cmt_wait:
			dpbparamstr="isc_dpb_wal_grp_cmt_wait";
			break;
		case isc_dpb_lc_messages:
			dpbparamstr="isc_dpb_lc_messages";
			break;
		case isc_dpb_lc_ctype:
			dpbparamstr="isc_dpb_lc_ctype";
			break;
		case isc_dpb_cache_manager:
			dpbparamstr="isc_dpb_cache_manager";
			break;
		case isc_dpb_shutdown:
			dpbparamstr="isc_dpb_shutdown";
			break;
		case isc_dpb_online:
			dpbparamstr="isc_dpb_online";
			break;
		case isc_dpb_shutdown_delay:
			dpbparamstr="isc_dpb_shutdown_delay";
			break;
		case isc_dpb_reserved:
			dpbparamstr="isc_dpb_reserved";
			break;
		case isc_dpb_overwrite:
			dpbparamstr="isc_dpb_overwrite";
			break;
		case isc_dpb_sec_attach:
			dpbparamstr="isc_dpb_sec_attach";
			break;
		case isc_dpb_disable_wal:
			dpbparamstr="isc_dpb_disable_wal";
			break;
		case isc_dpb_connect_timeout:
			dpbparamstr="isc_dpb_connect_timeout";
			break;
		case isc_dpb_dummy_packet_interval:
			dpbparamstr="isc_dpb_dummy_packet_interval";
			break;
		case isc_dpb_gbak_attach:
			dpbparamstr="isc_dpb_gbak_attach";
			break;
		case isc_dpb_sql_role_name:
			dpbparamstr="isc_dpb_sql_role_name";
			break;
		case isc_dpb_set_page_buffers:
			dpbparamstr="isc_dpb_set_page_buffers";
			break;
		case isc_dpb_working_directory:
			dpbparamstr="isc_dpb_working_directory";
			break;
		case isc_dpb_sql_dialect:
			dpbparamstr="isc_dpb_sql_dialect";
			break;
		case isc_dpb_set_db_readonly:
			dpbparamstr="isc_dpb_set_db_readonly";
			break;
		case isc_dpb_set_db_sql_dialect:
			dpbparamstr="isc_dpb_set_db_sql_dialect";
			break;
		case isc_dpb_gfix_attach:
			dpbparamstr="isc_dpb_gfix_attach";
			break;
		case isc_dpb_gstat_attach:
			dpbparamstr="isc_dpb_gstat_attach";
			break;
		case isc_dpb_set_db_charset:
			dpbparamstr="isc_dpb_set_db_charset";
			break;
		case isc_dpb_gsec_attach:
			dpbparamstr="isc_dpb_gsec_attach";
			break;
		case isc_dpb_address_path:
			dpbparamstr="isc_dpb_address_path";
			break;
		case isc_dpb_process_id:
			dpbparamstr="isc_dpb_process_id";
			break;
		case isc_dpb_no_db_triggers:
			dpbparamstr="isc_dpb_no_db_triggers";
			break;
		case isc_dpb_trusted_auth:
			dpbparamstr="isc_dpb_trusted_auth";
			break;
		case isc_dpb_process_name:
			dpbparamstr="isc_dpb_process_name";
			break;
		case isc_dpb_trusted_role:
			dpbparamstr="isc_dpb_trusted_role";
			break;
		case isc_dpb_org_filename:
			dpbparamstr="isc_dpb_org_filename";
			break;
		case isc_dpb_utf8_filename:
			dpbparamstr="isc_dpb_utf8_filename";
			break;
		case isc_dpb_ext_call_depth:
			dpbparamstr="isc_dpb_ext_call_depth";
			break;
		case isc_dpb_auth_block:
			dpbparamstr="isc_dpb_auth_block";
			break;
		case isc_dpb_client_version:
			dpbparamstr="isc_dpb_client_version";
			break;
		case isc_dpb_remote_protocol:
			dpbparamstr="isc_dpb_remote_protocol";
			break;
		case isc_dpb_host_name:
			dpbparamstr="isc_dpb_host_name";
			break;
		case isc_dpb_os_user:
			dpbparamstr="isc_dpb_os_user";
			break;
		case isc_dpb_specific_auth_data:
			dpbparamstr="isc_dpb_specific_auth_data";
			break;
		case isc_dpb_auth_plugin_list:
			dpbparamstr="isc_dpb_auth_plugin_list";
			break;
		case isc_dpb_auth_plugin_name:
			dpbparamstr="isc_dpb_auth_plugin_name";
			break;
		case isc_dpb_config:
			dpbparamstr="isc_dpb_config";
			break;
		case isc_dpb_nolinger:
			dpbparamstr="isc_dpb_nolinger";
			break;
		case isc_dpb_reset_icu:
			dpbparamstr="isc_dpb_reset_icu";
			break;
		case isc_dpb_map_attach:
			dpbparamstr="isc_dpb_map_attach";
			break;
		case isc_dpb_session_time_zone:
			dpbparamstr="isc_dpb_session_time_zone";
			break;
		case isc_dpb_set_db_replica:
			dpbparamstr="isc_dpb_set_db_replica";
			break;
		case isc_dpb_set_bind:
			dpbparamstr="isc_dpb_set_bind";
			break;
		case isc_dpb_decfloat_round:
			dpbparamstr="isc_dpb_decfloat_round";
			break;
		case isc_dpb_decfloat_traps:
			dpbparamstr="isc_dpb_decfloat_traps";
			break;
		case isc_dpb_clear_map:
			dpbparamstr="isc_dpb_clear_map";
			break;
		case isc_dpb_upgrade_db:
			dpbparamstr="isc_dpb_upgrade_db";
			break;
		case isc_dpb_parallel_workers:
			dpbparamstr="isc_dpb_parallel_workers";
			break;
		case isc_dpb_worker_attach:
			dpbparamstr="isc_dpb_worker_attach";
			break;
		case isc_dpb_owner:
			dpbparamstr="isc_dpb_owner";
			break;
		case isc_dpb_max_blob_cache_size:
			dpbparamstr="isc_dpb_max_blob_cache_size";
			break;
		case isc_dpb_max_inline_blob_size:
			dpbparamstr="isc_dpb_max_inline_blob_size";
			break;
		case isc_dpb_search_path:
			dpbparamstr="isc_dpb_search_path";
			break;
		case isc_dpb_blr_request_search_path:
			dpbparamstr="isc_dpb_blr_request_search_path";
			break;
		case isc_dpb_gbak_restore_has_schema:
			dpbparamstr="isc_dpb_gbak_restore_has_schema";
			break;
		default:
			dpbparamstr="unknown";
			break;
	}
	stdoutput.printf("	dpb param: %u (0x%02x) (%s)\n",
				dpbparam,dpbparam,dpbparamstr);
}

void sqlrprotocol_firebird::debugDbInfoItem(byte_t dbinfoitem) {
	if (!getDebug()) {
		return;
	}
	const char	*dbinfoitemstr=NULL;
	switch (dbinfoitem) {
		case isc_info_end:
			dbinfoitemstr="isc_info_end";
			break;
		case isc_info_db_id:
			dbinfoitemstr="isc_info_db_id";
			break;
		case isc_info_reads:
			dbinfoitemstr="isc_info_reads";
			break;
		case isc_info_writes:
			dbinfoitemstr="isc_info_writes";
			break;
		case isc_info_fetches:
			dbinfoitemstr="isc_info_fetches";
			break;
		case isc_info_marks:
			dbinfoitemstr="isc_info_marks";
			break;
		case isc_info_implementation:
			dbinfoitemstr="isc_info_implementation";
			break;
		case isc_info_isc_version:
			dbinfoitemstr="isc_info_isc_version";
			break;
		case isc_info_base_level:
			dbinfoitemstr="isc_info_base_level";
			break;
		case isc_info_page_size:
			dbinfoitemstr="isc_info_page_size";
			break;
		case isc_info_num_buffers:
			dbinfoitemstr="isc_info_num_buffers";
			break;
		case isc_info_limbo:
			dbinfoitemstr="isc_info_limbo";
			break;
		case isc_info_current_memory:
			dbinfoitemstr="isc_info_current_memory";
			break;
		case isc_info_max_memory:
			dbinfoitemstr="isc_info_max_memory";
			break;
		case isc_info_window_turns:
			dbinfoitemstr="isc_info_window_turns";
			break;
		case isc_info_license:
			dbinfoitemstr="isc_info_license";
			break;
		case isc_info_allocation:
			dbinfoitemstr="isc_info_allocation";
			break;
		case isc_info_attachment_id:
			dbinfoitemstr="isc_info_attachment_id";
			break;
		case isc_info_read_seq_count:
			dbinfoitemstr="isc_info_read_seq_count";
			break;
		case isc_info_read_idx_count:
			dbinfoitemstr="isc_info_read_idx_count";
			break;
		case isc_info_insert_count:
			dbinfoitemstr="isc_info_insert_count";
			break;
		case isc_info_update_count:
			dbinfoitemstr="isc_info_update_count";
			break;
		case isc_info_delete_count:
			dbinfoitemstr="isc_info_delete_count";
			break;
		case isc_info_backout_count:
			dbinfoitemstr="isc_info_backout_count";
			break;
		case isc_info_purge_count:
			dbinfoitemstr="isc_info_purge_count";
			break;
		case isc_info_expunge_count:
			dbinfoitemstr="isc_info_expunge_count";
			break;
		case isc_info_sweep_interval:
			dbinfoitemstr="isc_info_sweep_interval";
			break;
		case isc_info_ods_version:
			dbinfoitemstr="isc_info_ods_version";
			break;
		case isc_info_ods_minor_version:
			dbinfoitemstr="isc_info_ods_minor_version";
			break;
		case isc_info_no_reserve:
			dbinfoitemstr="isc_info_no_reserve";
			break;
		case isc_info_logfile:
			dbinfoitemstr="isc_info_logfile";
			break;
		case isc_info_cur_logfile_name:
			dbinfoitemstr="isc_info_cur_logfile_name";
			break;
		case isc_info_cur_log_part_offset:
			dbinfoitemstr="isc_info_cur_log_part_offset";
			break;
		case isc_info_num_wal_buffers:
			dbinfoitemstr="isc_info_num_wal_buffers";
			break;
		case isc_info_wal_buffer_size:
			dbinfoitemstr="isc_info_wal_buffer_size";
			break;
		case isc_info_wal_ckpt_length:
			dbinfoitemstr="isc_info_wal_ckpt_length";
			break;
		case isc_info_wal_cur_ckpt_interval:
			dbinfoitemstr="isc_info_wal_cur_ckpt_interval";
			break;
		case isc_info_wal_prv_ckpt_fname:
			dbinfoitemstr="isc_info_wal_prv_ckpt_fname";
			break;
		case isc_info_wal_prv_ckpt_poffset:
			dbinfoitemstr="isc_info_wal_prv_ckpt_poffset";
			break;
		case isc_info_wal_recv_ckpt_fname:
			dbinfoitemstr="isc_info_wal_recv_ckpt_fname";
			break;
		case isc_info_wal_recv_ckpt_poffset:
			dbinfoitemstr="isc_info_wal_recv_ckpt_poffset";
			break;
		case isc_info_wal_grpc_wait_usecs:
			dbinfoitemstr="isc_info_wal_grpc_wait_usecs";
			break;
		case isc_info_wal_num_io:
			dbinfoitemstr="isc_info_wal_num_io";
			break;
		case isc_info_wal_avg_io_size:
			dbinfoitemstr="isc_info_wal_avg_io_size";
			break;
		case isc_info_wal_num_commits:
			dbinfoitemstr="isc_info_wal_num_commits";
			break;
		case isc_info_wal_avg_grpc_size:
			dbinfoitemstr="isc_info_wal_avg_grpc_size";
			break;
		case isc_info_forced_writes:
			dbinfoitemstr="isc_info_forced_writes";
			break;
		case isc_info_user_names:
			dbinfoitemstr="isc_info_user_names";
			break;
		case isc_info_page_errors:
			dbinfoitemstr="isc_info_page_errors";
			break;
		case isc_info_record_errors:
			dbinfoitemstr="isc_info_record_errors";
			break;
		case isc_info_bpage_errors:
			dbinfoitemstr="isc_info_bpage_errors";
			break;
		case isc_info_dpage_errors:
			dbinfoitemstr="isc_info_dpage_errors";
			break;
		case isc_info_ipage_errors:
			dbinfoitemstr="isc_info_ipage_errors";
			break;
		case isc_info_ppage_errors:
			dbinfoitemstr="isc_info_ppage_errors";
			break;
		case isc_info_tpage_errors:
			dbinfoitemstr="isc_info_tpage_errors";
			break;
		case isc_info_set_page_buffers:
			dbinfoitemstr="isc_info_set_page_buffers";
			break;
		case isc_info_db_sql_dialect:
			dbinfoitemstr="isc_info_db_sql_dialect";
			break;
		case isc_info_db_read_only:
			dbinfoitemstr="isc_info_db_read_only";
			break;
		case isc_info_db_size_in_pages:
			dbinfoitemstr="isc_info_db_size_in_pages";
			break;
		case frb_info_att_charset:
			dbinfoitemstr="frb_info_att_charset";
			break;
		case isc_info_db_class:
			dbinfoitemstr="isc_info_db_class";
			break;
		case isc_info_firebird_version:
			dbinfoitemstr="isc_info_firebird_version";
			break;
		case isc_info_oldest_transaction:
			dbinfoitemstr="isc_info_oldest_transaction";
			break;
		case isc_info_oldest_active:
			dbinfoitemstr="isc_info_oldest_active";
			break;
		case isc_info_oldest_snapshot:
			dbinfoitemstr="isc_info_oldest_snapshot";
			break;
		case isc_info_next_transaction:
			dbinfoitemstr="isc_info_next_transaction";
			break;
		case isc_info_db_provider:
			dbinfoitemstr="isc_info_db_provider";
			break;
		case isc_info_active_transactions:
			dbinfoitemstr="isc_info_active_transactions";
			break;
		case isc_info_active_tran_count:
			dbinfoitemstr="isc_info_active_tran_count";
			break;
		case isc_info_creation_date:
			dbinfoitemstr="isc_info_creation_date";
			break;
		case isc_info_db_file_size:
			dbinfoitemstr="isc_info_db_file_size";
			break;
		case fb_info_page_contents:
			dbinfoitemstr="fb_info_page_contents";
			break;
		case fb_info_implementation:
			dbinfoitemstr="fb_info_implementation";
			break;
		case fb_info_page_warns:
			dbinfoitemstr="fb_info_page_warns";
			break;
		case fb_info_record_warns:
			dbinfoitemstr="fb_info_record_warns";
			break;
		case fb_info_bpage_warns:
			dbinfoitemstr="fb_info_bpage_warns";
			break;
		case fb_info_dpage_warns:
			dbinfoitemstr="fb_info_dpage_warns";
			break;
		case fb_info_ipage_warns:
			dbinfoitemstr="fb_info_ipage_warns";
			break;
		case fb_info_ppage_warns:
			dbinfoitemstr="fb_info_ppage_warns";
			break;
		case fb_info_tpage_warns:
			dbinfoitemstr="fb_info_tpage_warns";
			break;
		case fb_info_pip_errors:
			dbinfoitemstr="fb_info_pip_errors";
			break;
		case fb_info_pip_warns:
			dbinfoitemstr="fb_info_pip_warns";
			break;
		case fb_info_pages_used:
			dbinfoitemstr="fb_info_pages_used";
			break;
		case fb_info_pages_free:
			dbinfoitemstr="fb_info_pages_free";
			break;
		case fb_info_ses_idle_timeout_db:
			dbinfoitemstr="fb_info_ses_idle_timeout_db";
			break;
		case fb_info_ses_idle_timeout_att:
			dbinfoitemstr="fb_info_ses_idle_timeout_att";
			break;
		case fb_info_ses_idle_timeout_run:
			dbinfoitemstr="fb_info_ses_idle_timeout_run";
			break;
		case fb_info_conn_flags:
			dbinfoitemstr="fb_info_conn_flags";
			break;
		case fb_info_crypt_key:
			dbinfoitemstr="fb_info_crypt_key";
			break;
		case fb_info_crypt_state:
			dbinfoitemstr="fb_info_crypt_state";
			break;
		case fb_info_statement_timeout_db:
			dbinfoitemstr="fb_info_statement_timeout_db";
			break;
		case fb_info_statement_timeout_att:
			dbinfoitemstr="fb_info_statement_timeout_att";
			break;
		case fb_info_protocol_version:
			dbinfoitemstr="fb_info_protocol_version";
			break;
		case fb_info_crypt_plugin:
			dbinfoitemstr="fb_info_crypt_plugin";
			break;
		case fb_info_creation_timestamp_tz:
			dbinfoitemstr="fb_info_creation_timestamp_tz";
			break;
		case fb_info_wire_crypt:
			dbinfoitemstr="fb_info_wire_crypt";
			break;
		case fb_info_features:
			dbinfoitemstr="fb_info_features";
			break;
		case fb_info_next_attachment:
			dbinfoitemstr="fb_info_next_attachment";
			break;
		case fb_info_next_statement:
			dbinfoitemstr="fb_info_next_statement";
			break;
		case fb_info_db_guid:
			dbinfoitemstr="fb_info_db_guid";
			break;
		case fb_info_db_file_id:
			dbinfoitemstr="fb_info_db_file_id";
			break;
		case fb_info_replica_mode:
			dbinfoitemstr="fb_info_replica_mode";
			break;
		case fb_info_username:
			dbinfoitemstr="fb_info_username";
			break;
		case fb_info_sqlrole:
			dbinfoitemstr="fb_info_sqlrole";
			break;
		case fb_info_parallel_workers:
			dbinfoitemstr="fb_info_parallel_workers";
			break;
		case fb_info_wire_out_packets:
			dbinfoitemstr="fb_info_wire_out_packets";
			break;
		case fb_info_wire_in_packets:
			dbinfoitemstr="fb_info_wire_in_packets";
			break;
		case fb_info_wire_out_bytes:
			dbinfoitemstr="fb_info_wire_out_bytes";
			break;
		case fb_info_wire_in_bytes:
			dbinfoitemstr="fb_info_wire_in_bytes";
			break;
		case fb_info_wire_snd_packets:
			dbinfoitemstr="fb_info_wire_snd_packets";
			break;
		case fb_info_wire_rcv_packets:
			dbinfoitemstr="fb_info_wire_rcv_packets";
			break;
		case fb_info_wire_snd_bytes:
			dbinfoitemstr="fb_info_wire_snd_bytes";
			break;
		case fb_info_wire_rcv_bytes:
			dbinfoitemstr="fb_info_wire_rcv_bytes";
			break;
		case fb_info_wire_roundtrips:
			dbinfoitemstr="fb_info_wire_roundtrips";
			break;
		case fb_info_max_blob_cache_size:
			dbinfoitemstr="fb_info_max_blob_cache_size";
			break;
		case fb_info_max_inline_blob_size:
			dbinfoitemstr="fb_info_max_inline_blob_size";
			break;
		case fb_info_counts_scope_att:
			dbinfoitemstr="fb_info_counts_scope_att";
			break;
		case fb_info_counts_scope_db:
			dbinfoitemstr="fb_info_counts_scope_db";
			break;
		default:
			dbinfoitemstr="unknown";
			break;
	}
	stdoutput.printf("	info item: %u (0x%02x) (%s)\n",
				dbinfoitem,dbinfoitem,dbinfoitemstr);
}

void sqlrprotocol_firebird::debugTpbVersion(byte_t tpbversion) {
	if (!getDebug()) {
		return;
	}
	const char	*tpbversionstr=NULL;
	switch (tpbversion) {
		case isc_tpb_version1:
			tpbversionstr="isc_tpb_version1";
			break;
		case isc_tpb_version3:
			tpbversionstr="isc_tpb_version3";
			break;
		default:
			tpbversionstr="unknown";
			break;
	}
	stdoutput.printf("	tpb version: %u (%s)\n",
				tpbversion,tpbversionstr);
}

void sqlrprotocol_firebird::debugTpbParam(byte_t tpbparam) {
	if (!getDebug()) {
		return;
	}
	const char	*tpbparamstr=NULL;
	switch (tpbparam) {
		case isc_tpb_consistency:
			tpbparamstr="isc_tpb_consistency";
			break;
		case isc_tpb_concurrency:
			tpbparamstr="isc_tpb_concurrency";
			break;
		case isc_tpb_shared:
			tpbparamstr="isc_tpb_shared";
			break;
		case isc_tpb_protected:
			tpbparamstr="isc_tpb_protected";
			break;
		case isc_tpb_exclusive:
			tpbparamstr="isc_tpb_exclusive";
			break;
		case isc_tpb_wait:
			tpbparamstr="isc_tpb_wait";
			break;
		case isc_tpb_nowait:
			tpbparamstr="isc_tpb_nowait";
			break;
		case isc_tpb_read:
			tpbparamstr="isc_tpb_read";
			break;
		case isc_tpb_write:
			tpbparamstr="isc_tpb_write";
			break;
		case isc_tpb_lock_read:
			tpbparamstr="isc_tpb_lock_read";
			break;
		case isc_tpb_lock_write:
			tpbparamstr="isc_tpb_lock_write";
			break;
		case isc_tpb_verb_time:
			tpbparamstr="isc_tpb_verb_time";
			break;
		case isc_tpb_commit_time:
			tpbparamstr="isc_tpb_commit_time";
			break;
		case isc_tpb_ignore_limbo:
			tpbparamstr="isc_tpb_ignore_limbo";
			break;
		case isc_tpb_read_committed:
			tpbparamstr="isc_tpb_read_committed";
			break;
		case isc_tpb_autocommit:
			tpbparamstr="isc_tpb_autocommit";
			break;
		case isc_tpb_rec_version:
			tpbparamstr="isc_tpb_rec_version";
			break;
		case isc_tpb_no_rec_version:
			tpbparamstr="isc_tpb_no_rec_version";
			break;
		case isc_tpb_restart_requests:
			tpbparamstr="isc_tpb_restart_requests";
			break;
		case isc_tpb_no_auto_undo:
			tpbparamstr="isc_tpb_no_auto_undo";
			break;
		case isc_tpb_lock_timeout:
			tpbparamstr="isc_tpb_lock_timeout";
			break;
		case isc_tpb_read_consistency:
			tpbparamstr="isc_tpb_read_consistency";
			break;
		case isc_tpb_at_snapshot_number:
			tpbparamstr="isc_tpb_at_snapshot_number";
			break;
		case isc_tpb_auto_release_temp_blobid:
			tpbparamstr="isc_tpb_auto_release_temp_blobid";
			break;
		case isc_tpb_lock_table_schema:
			tpbparamstr="isc_tpb_lock_table_schema";
			break;
		default:
			tpbparamstr="unknown";
			break;
	}
	stdoutput.printf("	tpb param: %u (0x%02x) (%s)\n",
				tpbparam,tpbparam,tpbparamstr);
}

void sqlrprotocol_firebird::debugStatusVector(uint32_t *sv,
						const char **svstr,
						uint8_t svlen) {
	if (!getDebug()) {
		return;
	}
	stdoutput.write("	status vector:\n");
	uint32_t	cluster=1;
	uint8_t		i=0;
	while (i<svlen) {
		stdoutput.printf("		cluster %u:\n",cluster);
		switch (sv[i]) {
			case isc_arg_end:
				stdoutput.write("			"
						"code: isc_arg_end\n");
				i++;
				break;
			case isc_arg_gds:
				stdoutput.write("			"
						"code: isc_arg_gds\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_string:
				stdoutput.write("			"
						"code: isc_arg_string\n");
				i++;
				stdoutput.printf("			"
						"string: %s\n",
						(svstr[i])?svstr[i]:"");
				i++;
				break;
			case isc_arg_cstring:
				stdoutput.write("			"
						"code: isc_arg_cstring\n");
				i++;
				stdoutput.printf("			"
						"length: %u\n",sv[i]);
				i++;
				stdoutput.printf("			"
						"address: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_number:
				stdoutput.write("			"
						"code: isc_arg_number\n");
				i++;
				stdoutput.printf("			"
						"number: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_interpreted:
				stdoutput.write("			"
						"code: isc_arg_interpreted\n");
				i++;
				stdoutput.printf("			"
						"string: %s\n",
						(svstr[i])?svstr[i]:"");
				i++;
				break;
			case isc_arg_vms:
				stdoutput.write("			"
						"code: isc_arg_vms\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_unix:
				stdoutput.write("			"
						"code: isc_arg_unix\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_domain:
				stdoutput.write("			"
						"code: isc_arg_domain\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_dos:
				stdoutput.write("			"
						"code: isc_arg_dos\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_mpexl:
				stdoutput.write("			"
						"code: isc_arg_mpexl\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_mpexl_ipc:
				stdoutput.write("			"
						"code: isc_arg_mpexl_ipc\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_next_mach:
				stdoutput.write("			"
						"code: isc_arg_next_mach\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_netware:
				stdoutput.write("			"
						"code: isc_arg_netware\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_win32:
				stdoutput.write("			"
						"code: isc_arg_win32\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_warning:
				stdoutput.write("			"
						"code: isc_arg_warning\n");
				i++;
				stdoutput.printf("			"
						"warning: %u\n",sv[i]);
				i++;
				break;
			case isc_arg_sql_state:
				stdoutput.write("			"
						"code: isc_arg_sql_state\n");
				i++;
				stdoutput.printf("			"
						"sql state: %u\n",sv[i]);
				i++;
				break;
			default:
				stdoutput.write("			"
						"code: unknown\n");
				i++;
				stdoutput.printf("			"
						"error: %u\n",sv[i]);
				i++;
				break;
		}
		cluster++;
	}
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrprotocol	*new_sqlrprotocol_firebird(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrprotocol_firebird(cont,parameters);
	}
}
