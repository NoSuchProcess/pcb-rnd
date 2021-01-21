#!/bin/sh

awk '
	BEGIN {
		next_pid = 0
		next_edge = 0
	}

	{ sub("cdt:[^ ]* ", "", $0); }

# ins_point 41.9099999999999966 10.7949999999999999 0x5600efd1e6e0
	(($1 == "ins_point") || ($1 == "init_point")) {
		idx = $4
		pid = PID[idx];
		if (pid == "") {
			pid = next_pid
			next_pid++
			PID[idx] = pid
		}
		print "ins_point p" pid, $2, $3
		next
	}

# ins_cedge 0x5600efd1f7e0 0x5600efd1fab0 0x5600efd1de50
	($1 == "ins_cedge") {
		p1 = PID[$2]
		p2 = PID[$3]
		if (p1 == "") {
			print "invalid cedge point 1 ptr in " NR ": " $2 > "/dev/stderr"
			exit(1)
		}
		if (p2 == "") {
			print "invalid cedge point 2 ptr in " NR ": " $3 > "/dev/stderr"
			exit(1)
		}
		idx = $4
		eid = EID[idx];
		if (eid == "") {
			eid = next_eid
			next_eid++
			EID[idx] = eid
		}
		print "ins_cedge p" p1 " p" p2
		next
	}

	{ print $0 }

' < CDT
