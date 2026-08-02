#!/bin/sh

# run with tdsprotocoltlsmutualauthtest instance
#
# This is the mutual-auth counterpart to mysqltlsmutualauth.sh and
# postgresqltlsmutualauth.sh, but it cannot be made to pass.  Freetds
# 1.3.3 has no way to present a client certificate: the only x509 calls
# its libraries import are the server-verification ones, and no
# freetds.conf directive names a client cert or key.  Its
# CS_SEC_MUTUALAUTH is Kerberos and GSSAPI, not TLS.
#
# So this reports skipped rather than failed - sqlrelay is not at fault,
# the client simply cannot do it.  Re-check if freetds gains client
# certificate support.

echo "skipped - freetds cannot present a client certificate"
echo "  no gnutls_certificate_set_x509_key_file in libct, libsybdb or libtdsodbc"
echo "  no freetds.conf directive for a client cert or key"
echo "  CS_SEC_MUTUALAUTH is kerberos/gssapi, not tls"
exit 77
