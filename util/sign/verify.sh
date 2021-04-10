#!/bin/sh

### defaults ###
ca=rootca_repo.hu_2020.crt
crl=crl_repo.hu.pem

### implementation ###

verify()
{
	local base="$1" sig="$1.sig" crt="$1.crt"

	# check if all files are in place
	if test ! -f "$base"
	then
		echo "Content file $base does not exist" >&2
		return 1
	fi

	if test ! -f "$sig"
	then
		echo "Signature file $sig does not exist - please download that too!" >&2
		return 1
	fi

	if test ! -f "$crt"
	then
		echo "Certificate file $crt does not exist - please download that too!" >&2
		return 1
	fi

	if test ! -f "$ca"
	then
		echo "repo.hu CA cert $ca not found. Please download it from multiple sources" >&2
		return 1
	fi

	if test ! -f "$crl"
	then
		echo "repo.hu CRL $crl not found. Please download it from multiple sources" >&2
		return 1
	fi

	# step 4: check file's cert against repo.hu CA and CRL
	cat $ca $crl > .ca_crl.pem
	openssl verify -crl_check -CAfile .ca_crl.pem $crt >&2
	if test $? != 0
	then
		return 1
	fi

	# step 5: check file's sig against file's cert
	openssl x509 -pubkey -noout -in $crt > $crt.pem && \
	openssl dgst -verify $crt.pem -signature $sig $base >&2
}

fingerprint()
{
	echo -n "CA  "
	openssl x509 -fingerprint -noout -in $ca
	echo -n "CRL "
	openssl crl -fingerprint -noout -in $crl
}

print_file()
{
	case "$1" in
		*.crt) openssl x509 -in "$1" -text ;;
		crl*.pem) openssl crl -in "$1" -text ;;
		*) echo "Don't know how to print that file" ;;
	esac
}

help()
{
echo 'verify.sh - pcb-rnd download signature verify script
Usage: ./verify.sh [--fp] [--ca CA_file] [--crl CRL_file] downloaded_file

Download the CA and CRL files from multiple sources to make sure they match.
Altneratively print the the fingerprint with --fp and check that from
multiple sources.

The verification process is documented at

http://www.repo.hu/cgi-bin/pool.cgi?cmd=show&node=sign

Remember:

 - if you run this for the first time, you absolutely need to verify the CA
   from multiple sources

 - before running the script, please get the latest CRL, from multiple sources

The effective security of this process largely depends on these two steps.
'
}

### main ###

bad=0
while test $# -gt 0
do
	case "$1" in
		--help) help "$@"; exit;;
		--fp)  fingerprint ;;
		--ca) shift 1; ca="$1";;
		--crl) shift 1; crl="$1";;
		--print) shift 1; print_file "$1" ;;
		*)
			verify "$1"
			if test $? = 0
			then
				echo "*** $1: ok"
			else
				echo "*** $1: FAILED"
				bad=1
			fi
	esac

	shift 1
done

exit $bad
