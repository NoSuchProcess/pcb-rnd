echo === initial state: empty ===
reset *
dump native rc/library_search_paths

echo === import 3 levels ===
paste system \
	li:pcb-rnd-conf-v1 { \
		ha:overwrite { \
			ha:rc { \
				li:library_search_paths = { sys1; sys2 } \
			} \
		} \
	}
dump native rc/library_search_paths

paste user \
	li:pcb-rnd-conf-v1 { \
		ha:overwrite { \
			ha:rc { \
				li:library_search_paths = { user1; user2 } \
			} \
		} \
	}
dump native rc/library_search_paths

paste design \
	li:pcb-rnd-conf-v1 { \
		ha:overwrite { \
			ha:rc { \
				li:library_search_paths = { design1; design2 } \
			} \
		} \
	}
dump native rc/library_search_paths

echo === change policies ===
role design
chpolicy prepend
dump native rc/library_search_paths

role user
chpolicy append
dump native rc/library_search_paths

role design
chpolicy append
dump native rc/library_search_paths
