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
// (only the message-format subset.  the module never sends or receives a
// request blr, just the message blr that describes a row or a parameter set.)
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

// how many 1/10000ths of a second an ISC_TIME counts to the second
#define FIREBIRD_TIME_PRECISION	10000

// the largest day count decodeDate() will take
// (firebird's own ceiling is 31 December 9999, and its decode multiplies the
// count by four, so anything past that has to be refused rather than
// overflowed)
#define MAX_FIREBIRD_DATE	5373484

// how wide a bind variable says it is
// (SQL Relay only knows a bind's position, never its type, so a bind
// describes as a string, and this is the width it claims - big enough for
// anything a client is likely to send, small enough that a client sizing a
// buffer per parameter doesn't notice)
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
#define isc_bad_trans_handle		335544332
#define isc_convert_error		335544334
#define isc_infunk			335544341
#define isc_io_error			335544344
#define isc_no_dup			335544349
#define isc_read_only_trans		335544361
#define isc_wish_list			335544378
#define isc_random			335544382
#define isc_sqlerr			335544436
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
// (which dpb item the password came out of - see attach())
#define FIREBIRD_CLEARTEXT	"firebird_cleartext"
#define FIREBIRD_LEGACY		"firebird_legacy"

// connection type
#define P_REQ_async	1

// the largest counted string or buffer the module will read
// (firebird's api types every length these carry as a short - see
// isc_attach_database and isc_database_info in ibase.h - and firebird's own
// decoder folds a sign-extended 0xffffffff back down to 0xffff)
#define MAX_CSTRING_LENGTH	65535

// one field of a message, as the blr that came with it describes it
struct sqlrfirebirdfield {
	byte_t		blrtype;
	int16_t		scale;
	uint16_t	subtype;
	uint16_t	length;
};

// what the module knows about a statement the client allocated
// (the statement handle is the cursor id plus one, so a handle is never 0,
// and no separate handle-to-cursor map is needed - see getStatement())
struct sqlrfirebirdstatement {
	uint32_t		stmttype;
	bool			prepared;
	// the result set was opened by prepareStatement(), so execute()
	// must not open it a second time - see prepareStatement()
	bool			preexecuted;
	bool			cursoropen;
	char			*cursorname;
	// the message format of the last op_fetch, kept because the client
	// only sends the blr on the first fetch of a cursor
	sqlrfirebirdfield	*outfields;
	uint16_t		outfieldcount;
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

		void	successStatusVector();
		void	errorStatusVector(uint32_t gdscode);
		// Builds the status vector a real firebird sends when it
		// can't open the database file.  2.5, 3.0 and 4.0 all send
		// it byte-for-byte, and the "open" and "No such file or
		// directory" arguments below are theirs, captured off the
		// wire.
		void	openErrorStatusVector(const char *file);
		bool	genericResponse(const char *title,
						uint32_t objecthandle,
						uint32_t objectid,
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
		uint32_t	statementType(const char *query);
		bool	isTransactionStatement(uint32_t stmttype);
		bool	isWriteStatement(uint32_t stmttype);
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

		bool	readBlr(sqlrfirebirdfield **fields,
					uint16_t *fieldcount,
					const char *name,
					uint32_t *bytesread);
		bool	parseBlr(const byte_t *blr,
					uint32_t blrlen,
					sqlrfirebirdfield **fields,
					uint16_t *fieldcount);
		bool	readMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *bytesread);
		bool	writeMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *byteswritten);
		bool	writeField(const sqlrfirebirdfield *fld,
					const char *value,
					uint64_t valuesize,
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

		void	capRespBufferLen();

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
		// the values of the vector's string arguments, indexed
		// alongside it - a non-null entry means that element is a
		// string rather than an int
		// (string literals or session-lifetime buffers - not owned,
		// and not freed)
		const char	*statusvectorstr[20];
		uint8_t		statusvectorlen;

		// the transaction handle handed to the client
		// (SQL Relay has one transaction per session, so there is one
		// handle, and it is only valid between op_transaction and the
		// commit or rollback that ends it)
		uint32_t	trhandle;
		bool		intransaction;
		// the tpb asked for autocommit, so the commit or rollback
		// that ends this transaction has nothing left to do
		bool		trautocommit;
		// The tpb asked for a read-only transaction.  SQL Relay has no
		// way to ask a backend for one, so the transaction underneath
		// is read-write and the module refuses writes itself - see
		// isWriteStatement().
		bool		trreadonly;

		// how many statements the module can hold at once, and their
		// state, indexed by cursor id
		uint16_t	maxcursorcount;
		sqlrfirebirdstatement	*statements;

		// "?1", "?2", ... - what a bind variable is called when the
		// wire format only gives its position
		char		**bindvarnames;
		int16_t		*bindvarnamesizes;

		// A fetched blob is answered with an id rather than the data,
		// and the id has to be unique within the session and non-zero.
		// The backend's own id isn't reachable through the server API,
		// so the module counts its own - see writeField().
		uint32_t	blobid;

		// where errorResponse() keeps the text it was handed, since
		// statusvectorstr doesn't own what it points at
		stringbuffer	errormessage;
		char		errorsqlstate[6];

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
	maxcursorcount=cont->getConfig()->getMaxCursors();

	// per-statement state, indexed by cursor id
	statements=new sqlrfirebirdstatement[maxcursorcount];
	for (uint16_t i=0; i<maxcursorcount; i++) {
		statements[i].cursorname=NULL;
		statements[i].outfields=NULL;
	}

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
	wd=NULL;
	dbhandle=0;
	trhandle=0;
	intransaction=false;
	trautocommit=false;
	trreadonly=false;
	blobid=0;
	errorsqlstate[0]='\0';
	respbufferlen=0;
	for (uint16_t i=0; i<maxcursorcount; i++) {
		statements[i].stmttype=0;
		statements[i].prepared=false;
		statements[i].preexecuted=false;
		statements[i].cursoropen=false;
		statements[i].cursorname=NULL;
		statements[i].outfields=NULL;
		statements[i].outfieldcount=0;
	}
}

void sqlrprotocol_firebird::free() {
	delete[] db;
	delete[] username;
	delete[] password;
	delete[] wd;
	clearStatements();
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
				case op_dummy:
				case op_start_and_receive:
				case op_start_send_and_receive:
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
				objecthandle,objectid,
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
	// isc_arg_string, which the client renders itself)
	statusvector[8]=isc_arg_interpreted;
	statusvectorstr[9]="No such file or directory";
	// end of vector...
	statusvector[10]=isc_arg_end;
	statusvectorlen=11;
}

bool sqlrprotocol_firebird::genericResponse(const char *title,
						uint32_t objecthandle,
						uint32_t objectid,
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
	// (an argument string is written the same way any other buffer is -
	// a length, the bytes, and padding to a 4-byte boundary)
	for (uint8_t i=0; i<svlen; i++) {
		if (svstr[i]) {
			if (!writeBuffer((const byte_t *)svstr[i],
					charstring::getLength(svstr[i]),
					"status vector string",
					&byteswritten)) {
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
		byteswritten+=sizeof(uint32_t);
	}
	if (getDebug()) {
		debugStatusVector(sv,svstr,svlen);
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

	// A real firebird resolves the path on its own filesystem and looks
	// up aliases.  Neither is available here - the file belongs to the
	// database the connection module attached to, on whatever machine
	// that is - so the comparison is of the strings, exactly.
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

	// A detach ends whatever is still open.  Firebird rejects a detach
	// with a live transaction, but SQL Relay's session teardown rolls one
	// back on its own, so there's nothing to gain by refusing here.
	clearStatements();
	if (intransaction) {
		cont->rollback();
		intransaction=false;
		trautocommit=false;
		trreadonly=false;
		trhandle=0;
	}

	successStatusVector();

	// The client sends op_disconnect straight after this and then closes
	// the socket, so the session ends either way.  Answering the detach
	// and staying in the loop is what lets it.
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
	capRespBufferLen();

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
				// The module hands bytes through without
				// transliterating them, so the attachment's
				// character set is NONE.  Answering this one
				// matters more than it looks: isql treats an
				// isc_info_error for any item other than
				// isc_info_firebird_version as proof it is
				// talking to a pre-interbase-6 server, and
				// resets its sql dialect to 1.
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
				// a classic-mode server.  A real server always
				// names someone, so an attach that carried no
				// user name in its dpb stands in the same one
				// describeColumns() does.
				fits=appendInfoCountedString(dbinfoitem,
					(charstring::isNullOrEmpty(username))?
							"SYSDBA":username);
				break;

			default:
				// an item a real server wouldn't answer
				// either - isc_info_window_turns,
				// isc_info_license, and the wal and log-file
				// families that predate firebird.  Nothing
				// else lands here, on purpose - see the isql
				// note on frb_info_att_charset above.
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

	// get each parameter...
	// (the test is < rather than != because an item whose length walks
	// past the end would otherwise never land on the end pointer, and the
	// loop would run off the buffer - the bug #8967 fixed in the dpb walk)
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
				// isc_tpb_consistency, isc_tpb_concurrency,
				// isc_tpb_read_committed and the rest are bare
				// bytes.  SQL Relay has no way to ask the
				// backend for a particular isolation level per
				// transaction, so they are read and dropped.
				break;
		}
	}

	debugEnd();

	// clean up
	delete[] tpb;

	// SQL Relay has one transaction per session, and a client is free to
	// ask for several - isql asks for one in sql and then asks again with
	// this op.  Rather than refuse, every handle it asks for names the
	// same underlying transaction, and whichever one it commits or rolls
	// back ends it.
	if (!intransaction) {

		// Autocommit and an explicit transaction are alternatives.  A
		// tpb that asks for autocommit turns the connection's
		// autocommit on and begins nothing, and the commit or
		// rollback that ends it has nothing left to do.
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
	}

	// A handle is only ever compared against 0 by the client, but making
	// each one distinct keeps a stale handle from looking live in a debug
	// log.
	trhandle++;

	if (getDebug()) {
		stdoutput.printf("	transaction handle: %u\n",trhandle);
		stdoutput.printf("	read only: %s\n",(readonly)?"yes":"no");
		stdoutput.printf("	autocommit: %s\n",(autocommit)?"yes":"no");
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

	// A commit with nothing open is a no-op rather than an error.  The
	// client may hold a handle from a transaction something else already
	// ended - see transaction().
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

	intransaction=false;
	trautocommit=false;
	trreadonly=false;

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

	intransaction=false;
	trautocommit=false;
	trreadonly=false;

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
	capRespBufferLen();

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
				fits=appendInfoByte(trinfoitem,
						isc_info_tra_read_committed);
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

	// The name is kept but never handed to the backend - see #9087.  SQL
	// Relay has no way to name a backend cursor, and firebird only takes a
	// name between prepare and execute, so naming one here would be too
	// late anyway: runPreparedQuery() has already run a select with no
	// binds and opened the backend's cursor.  Naming and fetching works;
	// "where current of" fails at the backend with the -504 a real server
	// sends for a cursor that doesn't exist.
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

	// the length arrives sign-extended from a short often enough that
	// firebird's own decoder folds it back down
	if ((respbufferlen&0xffff0000)==0xffff0000) {
		respbufferlen&=0xffff;
	}
	capRespBufferLen();

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

	// A read-only tpb gets a read-write transaction from SQL Relay, which
	// has no way to ask a backend for a read-only one, so the write is
	// refused here instead.  This sits after the prepare, and only runs for
	// exec immediate, because that's where a real server refuses - a
	// prepare in a read-only transaction succeeds, and one against a table
	// that doesn't exist still has to fail with the backend's own error
	// rather than this one.  A real server sends nothing but a bare
	// isc_read_only_trans, and the client turns that one code into sqlcode
	// -817, sqlstate 42000 and "attempted update during read-only
	// transaction" out of its own tables.  It refuses at the moment a
	// record is actually modified, so an update or delete matching no rows
	// succeeds there and is refused here.
	if (execimmediate && trreadonly && isWriteStatement(stmttype)) {
		cont->release(cursor);
		return errorResponse(title,isc_read_only_trans);
	}

	// A firebird client expects op_prepare_statement to answer with the
	// shape of the result set, but SQL Relay only knows a query's columns
	// after it runs.  So a select with nothing to bind is run here, and
	// execute() knows not to run it a second time.  A select with binds
	// can't be, and describes as no columns.
	bool	executed=false;
	if (execimmediate ||
		((stmttype==isc_info_sql_stmt_select ||
			stmttype==isc_info_sql_stmt_select_for_upd) &&
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

	if (stmt) {
		stmt->stmttype=stmttype;
		stmt->prepared=true;
		stmt->preexecuted=executed;
		stmt->cursoropen=executed && cont->colCount(cursor)>0;
	}

	// build the reply the requested info items ask for
	respbuffer.clear();
	if (!execimmediate && itemslen) {
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
	// }

	debugStart((isexecute2)?"execute2":"execute");

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
	if (!readBlr(&infields,&infieldcount,"input blr",&bytesread)) {
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

	bool	messageread=true;
	if (inmsgcount && infieldcount) {
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
					"output blr",&bytesread)) {
			return false;
		}
		uint32_t	outmsgnumber;
		if (!readInt(&outmsgnumber,
				"output message number",&bytesread)) {
			delete[] outfields;
			return false;
		}
	}

	debugEnd();

	const char	*title=(isexecute2)?"execute2 response":
						"execute response";

	if (!stmt || !stmt->prepared) {
		delete[] outfields;
		return errorResponse(title,isc_bad_stmt_handle);
	}

	// refuse a write in a read-only transaction - see runPreparedQuery()
	if (trreadonly && isWriteStatement(stmt->stmttype)) {
		delete[] outfields;
		return errorResponse(title,isc_read_only_trans);
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
	if (!readBlr(&infields,&infieldcount,"input blr",&bytesread)) {
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

	// The message arrives ahead of the query it belongs to, so it has to
	// be bound to a cursor before there is anything to run on it.  Binds
	// live on the cursor rather than on the query, so filling them first
	// and preparing after works, as long as nothing clears them in
	// between - see runPreparedQuery().
	sqlrservercursor	*cursor=cont->getCursor();
	if (!cursor) {
		delete[] infields;
		return errorResponse("exec immediate2 response",
					isc_dsql_error,"HY000",-901,
					"Out of cursors",14);
	}
	cont->getBindPool(cursor)->clear();
	cont->setInputBindCount(cursor,0);

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
	if (!readBlr(&outfields,&outfieldcount,"output blr",&bytesread)) {
		cont->release(cursor);
		return false;
	}

	uint32_t	outmsgnumber;
	if (!readInt(&outmsgnumber,"output message number",&bytesread)) {
		delete[] outfields;
		cont->release(cursor);
		return false;
	}

	uint32_t	clienttrhandle;
	uint32_t	stmthandle;
	uint32_t	dialect;
	char		*query=NULL;
	uint32_t	querylen=0;
	uint32_t	itemslen=0;
	byte_t		*items=NULL;
	if (!readInt(&clienttrhandle,"transaction handle",&bytesread) ||
		!readInt(&stmthandle,"statement handle",&bytesread) ||
		!readInt(&dialect,"dialect",&bytesread) ||
		!readString(&query,&querylen,"query",&bytesread) ||
		!readBuffer(&items,&itemslen,
				"requested sql info items",&bytesread) ||
		!readInt(&respbufferlen,"response buffer length",&bytesread)) {
		delete[] query;
		delete[] items;
		delete[] outfields;
		cont->release(cursor);
		return false;
	}
	capRespBufferLen();

	debugEnd();

	bool	retval=runOnCursor(cursor,"exec immediate2 response",
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
	if (trreadonly && isWriteStatement(statementType(query))) {
		return errorResponse(title,isc_read_only_trans);
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
	if (!readBlr(&fields,&fieldcount,"output blr",&bytesread)) {
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

	// A batch that stopped because it filled up ends with status 0 and no
	// message, and one that ran out of rows ends with 100.
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

	if ((respbufferlen&0xffff0000)==0xffff0000) {
		respbufferlen&=0xffff;
	}
	capRespBufferLen();

	debugEnd();

	sqlrservercursor	*cursor=NULL;
	sqlrfirebirdstatement	*stmt=getStatement(stmthandle,&cursor);
	if (!stmt || !stmt->prepared) {
		delete[] items;
		return errorResponse("info sql response",isc_bad_stmt_handle);
	}

	respbuffer.clear();
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

bool sqlrprotocol_firebird::createBlob() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::createBlob2() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::openBlob() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::openBlob2() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::getSegment() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchSegment() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::seekBlob() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::cancelBlob() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::closeBlob() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::getSlice() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::putSlice() {
	return sendNotImplementedError();
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

	// A client sends this unsolicited, right after attach and again
	// mid-session, and reads no reply.  Answering it desynchronizes the
	// connection.  There is nothing to cancel either - the module never
	// has a query in flight while it is reading a request.
	return true;
}

bool sqlrprotocol_firebird::batchCreate() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchMsg() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchExec() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchRls() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchCancel() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchSync() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchSetBpb() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchRegBlob() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::batchBlobStream() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::serviceAttach() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::serviceDetach() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::serviceStart() {
	return sendNotImplementedError();
}

bool sqlrprotocol_firebird::serviceInfo() {
	return sendNotImplementedError();
}

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

	// An op the module doesn't implement still has its request sitting on
	// the socket, unread, so the connection can't be reused.  The session
	// ends either way - but it ends after the client has been told what
	// went wrong, rather than with a socket that just closed.
	errorResponse("not implemented response",
			isc_wish_list,"0A000",-901,
			"Feature is not supported",24);
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

	// The buffers the vector points into have to outlive this call, since
	// genericResponse() writes from them.  The message is copied by its
	// length rather than as a string - what the server API hands back is
	// a buffer with a size, and reading it as a string runs off the end
	// of the text into whatever the buffer still held.
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

	// The leading code is what the client renders as the first message and
	// what it looks the sql state up under.  An isc_arg_sql_state element
	// overrides the lookup, and the isc_sqlerr/isc_arg_number pair
	// overrides the sql code the same way, so the backend's own code
	// survives rather than being flattened to the leading code's.
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

	// The firebird backend reports the sql code, negative, which is what
	// a firebird client expects to read back out of the vector.  Any other
	// backend reports whatever it reports, and a positive number isn't a
	// sql code at all, so it gets the generic one.
	int32_t	sqlcode=(int32_t)errnum;
	if (sqlcode>=0) {
		sqlcode=-901;
	}

	// A real firebird leads an error from preparing a statement with
	// isc_dsql_error, which renders as "Dynamic SQL Error", and an error
	// from running one with whatever the failure itself was.  The module
	// can't reach for the individual runtime codes, so the second case
	// leads with isc_random and lets the backend's own text stand as the
	// message.
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
}

void sqlrprotocol_firebird::clearStatements() {
	for (uint16_t i=0; i<maxcursorcount; i++) {
		clearStatement(i);
	}
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
	return isc_info_sql_stmt_ddl;
}

bool sqlrprotocol_firebird::isTransactionStatement(uint32_t stmttype) {
	return (stmttype==isc_info_sql_stmt_start_trans ||
		stmttype==isc_info_sql_stmt_commit ||
		stmttype==isc_info_sql_stmt_rollback);
}

bool sqlrprotocol_firebird::isWriteStatement(uint32_t stmttype) {

	// Only these three, and deliberately not the ddl type.  statementType()
	// uses ddl as its catch-all, so "execute block" lands there, and a real
	// server runs a read-only "execute block" that only selects.  Real ddl
	// is refused by a real server, but with a compound reply carrying a dyn
	// code per verb and the object name - see #9107.  "set generator" is
	// allowed too, since generators are outside transaction control.
	return (stmttype==isc_info_sql_stmt_insert ||
		stmttype==isc_info_sql_stmt_update ||
		stmttype==isc_info_sql_stmt_delete);
}

bool sqlrprotocol_firebird::runTransactionStatement(uint32_t stmttype) {

	// A client can ask for a transaction in sql rather than with
	// op_transaction, and isql does - it sends "set transaction" as its
	// first statement and reads the new handle out of the response.
	// These never reach the backend.  SQL Relay drives its transaction
	// through the controller, and the sql itself would only fail on a
	// backend with no such statement.
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
			return true;

		default:
			return true;
	}
}

void sqlrprotocol_firebird::keepReading(int32_t sec, int32_t usec) {
	for (;;) {
		byte_t	buffer[1024];
		ssize_t	r=clientsock->read(&buffer,1024,sec,usec);
		if (getDebug()) {
			stdoutput.printf("read %lld more bytes...\n",
							(long long)r);
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

	// reject an out-of-bounds length, before it can size an allocation
	// (vallen+1 would also wrap to 0 at 0xffffffff)
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

	// reject an out-of-bounds length, before it can size an allocation
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

void sqlrprotocol_firebird::capRespBufferLen() {

	// The length never sizes an allocation - it's only the ceiling that
	// truncates the response - but a client that declares a huge one never
	// truncates, so the response buffer grows to whatever the requested
	// items produce.  Firebird's api types this length as a short, so a
	// cap costs a real client nothing, and clamping rather than failing
	// leaves the protocol's own truncation as the answer to too large an
	// ask.
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
					uint32_t *bytesread) {

	*fields=NULL;
	*fieldcount=0;

	uint32_t	blrlen=0;
	byte_t		*blr=NULL;
	if (!readBuffer(&blr,&blrlen,name,bytesread)) {
		return false;
	}

	bool	retval=parseBlr(blr,blrlen,fields,fieldcount);

	delete[] blr;

	return retval;
}

bool sqlrprotocol_firebird::parseBlr(const byte_t *blr,
					uint32_t blrlen,
					sqlrfirebirdfield **fields,
					uint16_t *fieldcount) {

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
		return false;
	}
	if (*p!=blr_begin || *(p+1)!=blr_message) {
		if (getDebug()) {
			stdoutput.write("	invalid blr message header\n");
		}
		return false;
	}
	p+=2;

	// message number
	p++;

	uint16_t	itemcount=(uint16_t)(p[0]|(p[1]<<8));
	p+=2;

	// Each column is a value item and a null indicator, so the count is
	// always even.  An odd one off the wire would round the array down and
	// leave the last item writing past its end.
	if (itemcount%2) {
		if (getDebug()) {
			stdoutput.printf("	odd blr item count: %u\n",
								itemcount);
		}
		return false;
	}

	uint16_t	count=itemcount/2;
	if (!count) {
		return true;
	}

	sqlrfirebirdfield	*f=new sqlrfirebirdfield[count];

	for (uint16_t i=0; i<itemcount; i++) {

		if (p>=end) {
			delete[] f;
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
				return false;
		}

		// the odd items are the null indicators, which are always a
		// blr_short, and carry nothing the module needs
		if (!(i%2)) {
			f[i/2]=fld;
		}
	}

	*fields=f;
	*fieldcount=count;

	if (getDebug()) {
		stdoutput.printf("	blr describes %u field(s)\n",count);
	}

	return true;
}

bool sqlrprotocol_firebird::readMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *bytesread) {

	// Everything the blr describes has to come off the socket, cursor or
	// no cursor, or the connection desynchronizes.  Only the binding is
	// conditional.
	memorypool		*bindpool=NULL;
	sqlrserverbindvar	*inbinds=NULL;
	if (cursor) {
		bindpool=cont->getBindPool(cursor);
		inbinds=cont->getInputBinds(cursor);
	}

	uint16_t	bindcount=0;

	for (uint16_t i=0; i<fieldcount; i++) {

		const sqlrfirebirdfield	*fld=&fields[i];

		// what came off the wire, in whichever of these the type uses
		int64_t		intval=0;
		double		dblval=0.0;
		uint32_t	dateval=0;
		uint32_t	timeval=0;
		char		*strval=NULL;
		uint32_t	strvallen=0;
		bool		isdate=false;
		bool		istime=false;

		switch (fld->blrtype) {

			case blr_short:
			case blr_long:
				{
				uint32_t	val=0;
				if (!readInt(&val,"parameter",bytesread)) {
					return false;
				}
				intval=(int32_t)val;
				}
				break;

			case blr_int64:
				{
				uint64_t	val=0;
				if (!readInt64(&val,"parameter",bytesread)) {
					return false;
				}
				intval=(int64_t)val;
				}
				break;

			case blr_quad:
			case blr_blob2:
				{
				uint32_t	high=0;
				uint32_t	low=0;
				if (!readInt(&high,"parameter",bytesread) ||
					!readInt(&low,"parameter",bytesread)) {
					return false;
				}
				}
				break;

			case blr_float:
				{
				uint32_t	val=0;
				if (!readInt(&val,"parameter",bytesread)) {
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
				if (!readInt64(&val,"parameter",bytesread)) {
					return false;
				}
				bytestring::copy(&dblval,&val,sizeof(dblval));
				}
				break;

			case blr_sql_date:
				if (!readInt(&dateval,"parameter",bytesread)) {
					return false;
				}
				isdate=true;
				break;

			case blr_sql_time:
				if (!readInt(&timeval,"parameter",bytesread)) {
					return false;
				}
				istime=true;
				break;

			case blr_timestamp:
				if (!readInt(&dateval,"parameter",bytesread) ||
					!readInt(&timeval,
						"parameter",bytesread)) {
					return false;
				}
				isdate=true;
				istime=true;
				break;

			case blr_bool:
				{
				byte_t	val=0;
				if (!readOpaque(&val,1,
						"parameter",bytesread)) {
					return false;
				}
				intval=val;
				}
				break;

			case blr_text:
			case blr_text2:
			case blr_cstring:
			case blr_cstring2:
				strvallen=fld->length;
				strval=new char[strvallen+1];
				if (!readOpaque((byte_t *)strval,strvallen,
						"parameter",bytesread)) {
					delete[] strval;
					return false;
				}
				strval[strvallen]='\0';
				break;

			case blr_varying:
			case blr_varying2:
				{
				uint32_t	len=0;
				if (!readInt(&len,
						"parameter length",bytesread)) {
					return false;
				}
				if (len>fld->length) {
					len=fld->length;
				}
				strvallen=len;
				strval=new char[strvallen+1];
				if (!readOpaque((byte_t *)strval,strvallen,
						"parameter",bytesread)) {
					delete[] strval;
					return false;
				}
				strval[strvallen]='\0';
				}
				break;

			default:
				// parseBlr() rejects anything else, so this
				// can't be reached
				return false;
		}

		// null indicator
		uint32_t	indicator=0;
		if (!readInt(&indicator,"null indicator",bytesread)) {
			delete[] strval;
			return false;
		}
		bool	isnull=((int32_t)indicator<0);

		if (!inbinds || bindcount>=maxbindcount) {
			delete[] strval;
			continue;
		}

		sqlrserverbindvar	*bv=&(inbinds[bindcount]);
		bv->variable=bindvarnames[bindcount];
		bv->variablesize=bindvarnamesizes[bindcount];

		if (isnull) {
			bv->type=SQLRSERVERBINDVARTYPE_NULL;
			bv->isnull=cont->getNullBindValue();
		} else if (strval) {
			bv->type=SQLRSERVERBINDVARTYPE_STRING;
			bv->valuesize=strvallen;
			bv->value.stringval=
				(char *)bindpool->allocate(strvallen+1);
			bytestring::copy(bv->value.stringval,
						strval,strvallen);
			bv->value.stringval[strvallen]='\0';
			bv->isnull=cont->getNonNullBindValue();
		} else if (isdate || istime) {
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
			if (isdate) {
				decodeDate(dateval,
						&bv->value.dateval.year,
						&bv->value.dateval.month,
						&bv->value.dateval.day);
			}
			if (istime) {
				decodeTime(timeval,
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
			bv->value.doubleval.value=dblval;
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
			bv->value.doubleval.value=(double)intval/divisor;
			bv->value.doubleval.precision=18;
			bv->value.doubleval.scale=-fld->scale;
			bv->isnull=cont->getNonNullBindValue();
		} else {
			bv->type=SQLRSERVERBINDVARTYPE_INTEGER;
			bv->value.integerval=intval;
			bv->isnull=cont->getNonNullBindValue();
		}

		delete[] strval;

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

bool sqlrprotocol_firebird::writeMessage(sqlrservercursor *cursor,
					sqlrfirebirdfield *fields,
					uint16_t fieldcount,
					uint32_t *byteswritten) {

	uint32_t	colcount=cont->colCount(cursor);

	for (uint16_t i=0; i<fieldcount; i++) {

		const char	*field=NULL;
		uint64_t	fieldsize=0;
		bool		lob=false;
		bool		null=true;

		if (i<colcount && !cont->getField(cursor,i,&field,
						&fieldsize,&lob,&null)) {
			return false;
		}

		if (!writeField(&fields[i],field,fieldsize,
					null,byteswritten)) {
			return false;
		}

		// A null value still occupies its full width above - this is
		// what actually says it is null.
		if (!writeInt((null)?0xffffffff:0,
					"null indicator",byteswritten)) {
			return false;
		}
	}

	return true;
}

bool sqlrprotocol_firebird::writeField(const sqlrfirebirdfield *fld,
					const char *value,
					uint64_t valuesize,
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
			// A fetched blob is answered with an id rather than
			// the data, and the backend's own id isn't reachable
			// through the server API, so the module hands out one
			// of its own.  Nothing can be fetched with it yet -
			// the blob ops are still unimplemented.
			uint32_t	low=0;
			if (!null) {
				blobid++;
				low=blobid;
			}
			return writeInt(0,"field",byteswritten) &&
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
			// the width includes the terminator, so a declared
			// width of 0 leaves no room for any value at all
			uint32_t	width=fld->length;
			uint32_t	max=(width)?width-1:0;
			byte_t		*buf=new byte_t[(width)?width:1];
			uint32_t	len=(vlen<max)?vlen:max;
			bytestring::zero(buf,(width)?width:1);
			bytestring::copy(buf,v,len);
			bool	retval=writeOpaque(buf,width,
							"field",byteswritten);
			delete[] buf;
			return retval;
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

		// Firebird has one lob type for everything, distinguished by
		// its sub type, so every character and binary lob maps onto
		// it.
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

	// For a number, the sub type is what tells decimal from numeric -
	// firebird stores both as a scaled integer and has nothing else to
	// tell them apart with.  For text it is the character set id, and for
	// a lob it is 0 for binary and 1 for text.
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
						bool bind,
						uint32_t start,
						const byte_t *items,
						uint32_t itemslen) {

	// how many columns the result set has, or how many parameters the
	// query binds
	uint32_t	count=(bind)?
			cont->countBindVariables(cont->getQueryBuffer(cursor),
						cont->getQuerySize(cursor)):
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

		// The sequence has to lead each group.  It is what says which
		// column the items after it describe, and a client that sees
		// it late rejects the whole reply.
		if (!appendInfoInt(isc_info_sql_sqlda_seq,col+1)) {
			return false;
		}

		// SQL Relay only knows a bind's position, never its type, so
		// a bind describes as a nullable string of a generous width -
		// the one shape every backend can convert from.
		uint16_t	coltype=(bind)?UNKNOWN_DATATYPE:
					cont->getColumnType(cursor,col);
		uint16_t	sqltype=sqlType(coltype);
		uint32_t	colsize=(bind)?FIREBIRD_BIND_LENGTH:
					cont->getColumnSize(cursor,col);
		int32_t		colscale=(bind)?0:
					-((int32_t)
					cont->getColumnScale(cursor,col));
		const char	*colname=(bind)?"":
					cont->getColumnName(cursor,col);
		const char	*coltable=(bind)?"":
					cont->getColumnTable(cursor,col);
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
					appendInfoDescribe(cursor,
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

	// A scaled number goes on the wire as an integer with the decimal
	// point left out - 1.5 at scale -2 is 150 - so the digits have to be
	// shifted rather than the value divided, or the last digit rounds
	// away.
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

	// The firebird connection module renders a date as yyyy:mm:dd, a time
	// as hh:mm:ss and a timestamp as "yyyy-mm-dd hh:mm:ss", so what
	// separates the numbers varies but their order never does.  Pulling
	// the numbers out in order sidesteps the ambiguity that a delimiter-
	// driven parse runs into with a colon-delimited date.
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
