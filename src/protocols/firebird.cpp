// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/character.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/process.h>
#include <rudiments/randomnumber.h>
#include <rudiments/file.h>
#include <rudiments/error.h>

#include <datatypes.h>

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
// 18 supports op_fetch_scroll
#define PROTOCOL_VERSION18	(0xffff8000|18)
// 19 supports op_inline_blob
#define PROTOCOL_VERSION19	(0xffff8000|19)
// 20 supports prepare flags
#define PROTOCOL_VERSION20	(0xffff8000|20)

// the highest version the module can negotiate
// (13 and up must answer op_accept_data or op_cond_accept and drive the auth
// plugin handshake, which the module doesn't implement - see #8947)
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
#define isc_infunk	335544341
#define isc_login	335544472

// what the module answers isc_database_info with
// (connect() caps protocol negotiation at 12, so the module presents itself
// as a firebird 2.5-era server - see MAX_PROTOCOL_VERSION)
#define FIREBIRD_PAGE_SIZE		4096
#define FIREBIRD_NUM_BUFFERS		75
#define FIREBIRD_ODS_VERSION		11
#define FIREBIRD_ODS_MINOR_VERSION	2
#define FIREBIRD_SQL_DIALECT		3
#define FIREBIRD_SWEEP_INTERVAL		20000
// classic access - one server process per attachment, like sqlr-connection
#define FIREBIRD_DB_CLASS		13
// isc_info_db_code_firebird
#define FIREBIRD_DB_PROVIDER		4

// authentication methods
// (which dpb item the password came out of - see attach())
#define FIREBIRD_CLEARTEXT	"firebird_cleartext"
#define FIREBIRD_LEGACY		"firebird_legacy"

// connection type
#define P_REQ_async	1

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

		void	successStatusVector();
		void	errorStatusVector(uint32_t gdscode);
		bool	genericResponse(const char *title,
						uint32_t objecthandle,
						uint32_t objectid,
						const byte_t *buffer,
						uint32_t bufferlen,
						uint32_t *sv,
						uint8_t svlen);

		bool	authenticate();

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
		bool	prepare();
		bool	prepare2();
		bool	transactionInfo();
		bool	allocateStatement();
		bool	freeStatement();
		bool	prepareStatement();
		bool	execute();
		bool	execute2();
		bool	fetch();
		bool	setCursor();
		bool	infoSql();
		bool	createBlob();
		bool	createBlob2();
		bool	openBlob();
		bool	openBlob2();
		bool	getSegment();
		bool	batchSegment();
		bool	seekBlob();
		bool	cancelBlob();
		bool	closeBlob();
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
		bool	readPadding(uint32_t *bytesread);

		bool	writeInt(uint32_t val,
					const char *name,
					uint32_t *byteswritten);
		bool	writeBuffer(const byte_t *val,
					uint32_t len,
					const char *name,
					uint32_t *byteswritten);

		bool	appendInfoItem(byte_t item,
					const byte_t *value,
					uint16_t valuelen);
		bool	appendInfoInt(byte_t item, uint32_t value);
		bool	appendInfoByte(byte_t item, byte_t value);
		bool	appendInfoStrings(byte_t item,
					const char * const *values,
					byte_t valuecount);
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
		void	debugStatusVector(uint32_t *sv, uint8_t svlen);

		uint32_t	maxquerysize;
		uint16_t	maxbindcount;

		filedescriptor	*clientsock;

		uint32_t	opcode;

		// what connect() negotiated (see MAX_PROTOCOL_VERSION)
		uint32_t	protocolversion;

		char		*db;
		char		*username;
		char		*password;
		// which dpb item the password came out of
		// (a string literal - not owned, and not freed)
		const char	*authmethod;
		char		*wd;
		uint32_t	dbhandle;

		uint32_t	statusvector[20];
		uint8_t		statusvectorlen;

		bytebuffer	respbuffer;

		// how big a response buffer the client is willing to accept
		uint32_t	respbufferlen;
};


sqlrprotocol_firebird::sqlrprotocol_firebird(sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrprotocol(cont,parameters) {

	clientsock=NULL;

	debugStart("parameters");
	debugEnd();

	maxquerysize=cont->getConfig()->getMaxQuerySize();
	maxbindcount=cont->getConfig()->getMaxBindCount();

	init();
}

sqlrprotocol_firebird::~sqlrprotocol_firebird() {
	free();
}

void sqlrprotocol_firebird::init() {
	protocolversion=0;
	db=NULL;
	username=NULL;
	password=NULL;
	authmethod=NULL;
	wd=NULL;
	dbhandle=0;
	respbufferlen=0;
}

void sqlrprotocol_firebird::free() {
	delete[] db;
	delete[] username;
	delete[] password;
	delete[] wd;
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
				case op_batch_segments:
					loop=batchSegment();
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
				case op_compile:
				case op_start:
				case op_start_and_send:
				case op_send:
				case op_receive:
				case op_unwind:
				case op_release:
				case op_reconnect:
				case op_put_segment:
				case op_info_request:
				case op_info_blob:
				case op_aux_connect:
				case op_ddl:
				case op_exec_immediate:
				case op_dummy:
				case op_start_and_receive:
				case op_start_send_and_receive:
				case op_exec_immediate2:
				case op_insert:
				case op_transact:
				case op_rollback_retaining:
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
				case op_info_batch:
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
	// FIXME: do something with the user id
	delete[] userid;

	// get protocols, keeping the best one we can speak
	bool		accepted=false;
	uint32_t	acptversion=0;
	uint32_t	acptarchtype=arch_generic;
	uint32_t	acpttype=0;
	uint32_t	acptweight=0;
	for (uint32_t i=0; i<protocount; i++) {

		if (getDebug()) {
			stdoutput.printf("	protocol %d...\n",i);
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

		// Only the first MAX_CNCT_VERSIONS offered protocols count, but
		// every one the client sent still has to be read, or the rest
		// of them would be misread as the next packet.  Firebird does
		// the same - it reads p_cnct_count tuples and then clamps
		// p_cnct_count.  See FB25 src/remote/protocol.cpp:294.
		if (i>=MAX_CNCT_VERSIONS) {
			continue;
		}

		// skip versions we can't speak
		if (protoversion!=PROTOCOL_VERSION10 &&
			(protoversion<PROTOCOL_VERSION11 ||
			protoversion>MAX_PROTOCOL_VERSION)) {
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

	// FIXME: PROTOCOL_VERSION13 and up must answer op_accept_data or
	// op_cond_accept here, and drive the auth plugin handshake (see #8947).
	// MAX_PROTOCOL_VERSION keeps the negotiation below 13 until they can.
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
	if (!readInt(&opcode,"attach op code",op_attach,&bytesread)) {
		return false;
	}
	debugOpCode("attach op code",opcode);

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

	// process db parameters buffer...
	const byte_t	*dpbptr=dpb;
	const byte_t	*dpbendptr=dpb+dpblen;

	// get the dpb version
	byte_t	dpbversion=0;
	if (dpbptr!=dpbendptr) {
		read(dpbptr,&dpbversion,&dpbptr);
		debugDpbVersion(dpbversion);
	}

	// The version byte selects the item encoding.  isc_dpb_version1 gives
	// each item a 1-byte value length, isc_dpb_version2 (the "wide"
	// variant, which firebird sends only at protocol 13 and up) a 4-byte
	// little-endian one.  Any other version byte leaves the framing
	// unknown, so the walk below is skipped rather than guessed at.
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
			stdoutput.printf("	dpb value length: %d\n",
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
				readStringFromBuffer(dpbvalue,dpbvaluelen,
							"user name",&username);
				break;

			// The password arrives one of two ways.  isc_dpb_password
			// is the password itself, and isc_dpb_password_enc is
			// firebird's legacy_auth hash of it.  fbclient rewrites
			// the former into the latter, so a real firebird client
			// always sends the hash; only clients that build the
			// dpb themselves send the password.
			case isc_dpb_password:
				readStringFromBuffer(dpbvalue,dpbvaluelen,
							"password",&password);
				authmethod=FIREBIRD_CLEARTEXT;
				break;

			case isc_dpb_password_enc:
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

			case isc_dpb_specific_auth_data:
				// FIXME: do something...
				break;

			case isc_dpb_auth_plugin_list:
				// FIXME: do something...
				break;

			case isc_dpb_auth_plugin_name:
				// FIXME: do something...
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

	// authenticate
	// (the credentials came out of the dpb above)
	if (!authenticate()) {
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
				objecthandle,objectid,
				NULL,0,
				statusvector,statusvectorlen);
}

void sqlrprotocol_firebird::successStatusVector() {
	bytestring::zero(statusvector,sizeof(statusvector));
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
	// interbase error...
	statusvector[0]=isc_arg_gds;
	// the error...
	statusvector[1]=gdscode;
	// end of vector...
	statusvector[2]=isc_arg_end;
	statusvectorlen=3;
}

bool sqlrprotocol_firebird::genericResponse(const char *title,
						uint32_t objecthandle,
						uint32_t objectid,
						const byte_t *buffer,
						uint32_t bufferlen,
						uint32_t *sv,
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

	// write the object id
	if (!writeInt(objectid,"object id",&byteswritten)) {
		return false;
	}

	// write the low word of the blob id
	// (an 8-byte blob id is always on the wire, even in a response that
	// has nothing to do with blobs, and the object id above is its high
	// word)
	if (!writeInt(0,"blob id low word",&byteswritten)) {
		return false;
	}

	// write the buffer
	if (!writeBuffer(buffer,bufferlen,"buffer",&byteswritten)) {
		return false;
	}

	// write the status vector
	for (uint8_t i=0; i<svlen; i++) {
		if (clientsock->write(sv[i])!=sizeof(uint32_t)) {
			if (getDebug()) {
				stdoutput.printf("	write status "
						"vector [%d] failed\n",i);
				debugSystemError();
				debugEnd();
			}
			return false;
		}
		byteswritten+=sizeof(uint32_t);
	}
	if (getDebug()) {
		debugStatusVector(sv,svlen);
	}

	debugEnd();

	clientsock->flushWriteBuffer(-1,-1);

	return true;
}

bool sqlrprotocol_firebird::authenticate() {

	// build auth credentials
	sqlrfirebirdcredentials	cred;
	cred.setUser(username);
	cred.setPassword(password);
	cred.setPasswordSize(charstring::getLength(password));
	cred.setMethod(authmethod);

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

	// A failed login is answered with an op_response carrying nothing but
	// isc_login, and no message text.  That is what a real firebird server
	// sends, and the client renders the text from the code itself.
	errorStatusVector(isc_login);

	// the response is sent, but the session still ends
	genericResponse("attach failure response",
				0,0,
				NULL,0,
				statusvector,statusvectorlen);
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
	return false;
}

bool sqlrprotocol_firebird::create() {
	return false;
}

bool sqlrprotocol_firebird::dropDatabase() {
	return false;
}

bool sqlrprotocol_firebird::appendInfoItem(byte_t item,
					const byte_t *value,
					uint16_t valuelen) {

	// A response buffer isn't filled to the brim.  The 4 is the item byte,
	// the 2 length bytes, and 1 more held back for the trailing
	// isc_info_end.  When the cluster doesn't fit, a bare isc_info_truncated
	// takes its place, the reply gets no isc_info_end, and the status vector
	// still says success - the marker in the buffer is the only signal.
	// See INF_put_item.
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

			default:
				// FIXME: answer more of these - #7231.  Until
				// then, an item the module can't answer gets
				// isc_info_error, which is what a real server
				// sends for an item it doesn't recognize.
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
				dbhandle,objectid,
				respbuffer.getBuffer(),
				respbuffer.getSize(),
				statusvector,statusvectorlen);
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
		// FIXME: do something with this...
	}

	// get each parameter...
	while (tpbptr!=tpbendptr) {
		
		// get the parameter
		byte_t	tpbparam;
		read(tpbptr,&tpbparam,&tpbptr);
		debugTpbParam(tpbparam);

		// process the parameter...
		switch (tpbparam) {
			case isc_tpb_consistency:
				// FIXME: do something
				break;

			case isc_tpb_concurrency:
				// FIXME: do something
				break;

			case isc_tpb_shared:
				// FIXME: do something
				break;

			case isc_tpb_protected:
				// FIXME: do something
				break;

			case isc_tpb_exclusive:
				// FIXME: do something
				break;

			case isc_tpb_wait:
				// FIXME: do something
				break;

			case isc_tpb_nowait:
				// FIXME: do something
				break;

			case isc_tpb_read:
				// FIXME: do something
				break;

			case isc_tpb_write:
				// FIXME: do something
				break;

			case isc_tpb_lock_read:
				// FIXME: do something
				break;

			case isc_tpb_lock_write:
				// FIXME: do something
				break;

			case isc_tpb_verb_time:
				// FIXME: do something
				break;

			case isc_tpb_commit_time:
				// FIXME: do something
				break;

			case isc_tpb_ignore_limbo:
				// FIXME: do something
				break;

			case isc_tpb_read_committed:
				// FIXME: do something
				break;

			case isc_tpb_autocommit:
				// FIXME: do something
				break;

			case isc_tpb_rec_version:
				// FIXME: do something
				break;

			case isc_tpb_no_rec_version:
				// FIXME: do something
				break;

			case isc_tpb_restart_requests:
				// FIXME: do something
				break;

			case isc_tpb_no_auto_undo:
				// FIXME: do something
				break;

			case isc_tpb_lock_timeout:
				// FIXME: do something
				break;

			default:
				// FIXME: do something
				break;
		}
	}

	debugEnd();

	// clean up
	delete[] tpb;

	// increment the dbhandle, apparently???
	dbhandle++;

	// FIXME: object id should be the transaction handle???
	uint32_t	objectid=0;

	// status vector...
	successStatusVector();

	return genericResponse("transaction response",
				dbhandle,objectid,
				NULL,0,
				statusvector,statusvectorlen);
}

bool sqlrprotocol_firebird::commit() {
	return false;
}

bool sqlrprotocol_firebird::rollback() {
	return false;
}

bool sqlrprotocol_firebird::commitRetaining() {
	return false;
}

bool sqlrprotocol_firebird::prepare() {
	return false;
}

bool sqlrprotocol_firebird::prepare2() {
	return false;
}

bool sqlrprotocol_firebird::transactionInfo() {
	return false;
}

bool sqlrprotocol_firebird::allocateStatement() {
	return false;
}

bool sqlrprotocol_firebird::freeStatement() {
	return false;
}

bool sqlrprotocol_firebird::prepareStatement() {
	return false;
}

bool sqlrprotocol_firebird::execute() {
	return false;
}

bool sqlrprotocol_firebird::execute2() {
	return false;
}

bool sqlrprotocol_firebird::fetch() {
	return false;
}

bool sqlrprotocol_firebird::setCursor() {
	return false;
}

bool sqlrprotocol_firebird::infoSql() {
	return false;
}

bool sqlrprotocol_firebird::createBlob() {
	return false;
}

bool sqlrprotocol_firebird::createBlob2() {
	return false;
}

bool sqlrprotocol_firebird::openBlob() {
	return false;
}

bool sqlrprotocol_firebird::openBlob2() {
	return false;
}

bool sqlrprotocol_firebird::getSegment() {
	return false;
}

bool sqlrprotocol_firebird::batchSegment() {
	return false;
}

bool sqlrprotocol_firebird::seekBlob() {
	return false;
}

bool sqlrprotocol_firebird::cancelBlob() {
	return false;
}

bool sqlrprotocol_firebird::closeBlob() {
	return false;
}

bool sqlrprotocol_firebird::getSlice() {
	return false;
}

bool sqlrprotocol_firebird::putSlice() {
	return false;
}

bool sqlrprotocol_firebird::cancel() {
	return false;
}

bool sqlrprotocol_firebird::batchCreate() {
	return false;
}

bool sqlrprotocol_firebird::batchMsg() {
	return false;
}

bool sqlrprotocol_firebird::batchExec() {
	return false;
}

bool sqlrprotocol_firebird::batchRls() {
	return false;
}

bool sqlrprotocol_firebird::batchCancel() {
	return false;
}

bool sqlrprotocol_firebird::batchSync() {
	return false;
}

bool sqlrprotocol_firebird::batchSetBpb() {
	return false;
}

bool sqlrprotocol_firebird::batchRegBlob() {
	return false;
}

bool sqlrprotocol_firebird::batchBlobStream() {
	return false;
}

bool sqlrprotocol_firebird::serviceAttach() {
	return false;
}

bool sqlrprotocol_firebird::serviceDetach() {
	return false;
}

bool sqlrprotocol_firebird::serviceStart() {
	return false;
}

bool sqlrprotocol_firebird::serviceInfo() {
	return false;
}

bool sqlrprotocol_firebird::connectRequest() {
	return false;
}

bool sqlrprotocol_firebird::queEvents() {
	return false;
}

bool sqlrprotocol_firebird::cancelEvents() {
	return false;
}

bool sqlrprotocol_firebird::sendNotImplementedError() {
	return false;
}

void sqlrprotocol_firebird::keepReading(int32_t sec, int32_t usec) {
	for (;;) {
		byte_t	buffer[1024];
		ssize_t	r=clientsock->read(&buffer,1024,sec,usec);
		if (getDebug()) {
			stdoutput.printf("read %d more bytes...\n",r);
		}
		if (r<1) {
			break;
		}
		debugHexDump(buffer,r);
	}
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
		stdoutput.printf("	%s: %d\n",name,*val);
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
						"got %d, expected %d\n",
						name,*val,expected);
			debugEnd();
		}
		return false;
	}
	return true;
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
		stdoutput.printf("	%s length: %d\n",name,vallen);
	}
	(*bytesread)+=sizeof(uint32_t);

	// init buffer
	*val=NULL;

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
	return readPadding(bytesread);
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
		stdoutput.printf("	%s length: %d\n",name,vallen);
	}
	(*bytesread)+=sizeof(uint32_t);

	// init buffer
	*val=NULL;

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
	return readPadding(bytesread);
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

	// bail if we don't need to read any padding
	if (!pad) {
		return true;
	}

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
		stdoutput.printf("	(%d bytes of padding)\n",pad);
	}
	return true;
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
		stdoutput.printf("	%s: %d\n",name,val);
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
		stdoutput.printf("	%s len: %d\n",name,len);
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
		stdoutput.printf("	(%d bytes of padding)\n",pad);
	}
	return true;
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
	stdoutput.printf("	connect version: %d (%s)\n",
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
		stdoutput.printf("	user id tag: %d (0x%02x) (%s)\n",
							tag,tag,tagstr);

		// get the value length
		byte_t	valuelen=0;
		read(ptr,&valuelen,&ptr);
		stdoutput.printf("	user id value length: %d\n",valuelen);

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
	stdoutput.printf("	dpb version: %d (%s)\n",
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
	stdoutput.printf("	dpb param: %d (0x%02x) (%s)\n",
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
	stdoutput.printf("	info item: %d (0x%02x) (%s)\n",
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
	stdoutput.printf("	tpb version: %d (%s)\n",
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
	stdoutput.printf("	tpb param: %d (0x%02x) (%s)\n",
				tpbparam,tpbparam,tpbparamstr);
}

void sqlrprotocol_firebird::debugStatusVector(uint32_t *sv, uint8_t svlen) {
	if (!getDebug()) {
		return;
	}
	stdoutput.write("	status vector:\n");
	uint32_t	cluster=1;
	uint8_t		i=0;
	while (i<svlen) {
		stdoutput.printf("		cluster %d:\n",cluster);
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
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_string:
				stdoutput.write("			"
						"code: isc_arg_string\n");
				i++;
				stdoutput.printf("			"
						"address: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_cstring:
				stdoutput.write("			"
						"code: isc_arg_cstring\n");
				i++;
				stdoutput.printf("			"
						"length: %d\n",sv[i]);
				i++;
				stdoutput.printf("			"
						"address: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_number:
				stdoutput.write("			"
						"code: isc_arg_number\n");
				i++;
				stdoutput.printf("			"
						"number: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_interpreted:
				stdoutput.write("			"
						"code: isc_arg_interpreted\n");
				i++;
				stdoutput.printf("			"
						"address: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_vms:
				stdoutput.write("			"
						"code: isc_arg_vms\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_unix:
				stdoutput.write("			"
						"code: isc_arg_unix\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_domain:
				stdoutput.write("			"
						"code: isc_arg_domain\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_dos:
				stdoutput.write("			"
						"code: isc_arg_dos\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_mpexl:
				stdoutput.write("			"
						"code: isc_arg_mpexl\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_mpexl_ipc:
				stdoutput.write("			"
						"code: isc_arg_mpexl_ipc\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_next_mach:
				stdoutput.write("			"
						"code: isc_arg_next_mach\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_netware:
				stdoutput.write("			"
						"code: isc_arg_netware\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_win32:
				stdoutput.write("			"
						"code: isc_arg_win32\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_warning:
				stdoutput.write("			"
						"code: isc_arg_warning\n");
				i++;
				stdoutput.printf("			"
						"warning: %d\n",sv[i]);
				i++;
				break;
			case isc_arg_sql_state:
				stdoutput.write("			"
						"code: isc_arg_sql_state\n");
				i++;
				stdoutput.printf("			"
						"sql state: %d\n",sv[i]);
				i++;
				break;
			default:
				stdoutput.write("			"
						"code: unknown\n");
				i++;
				stdoutput.printf("			"
						"error: %d\n",sv[i]);
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
