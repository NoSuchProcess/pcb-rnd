#define RBSR_SEQ_MAX 256

typedef struct rbsr_seq_s {
	rbsr_map_t map;
	grbs_2net_t *tn;

	grbs_addr_t path[RBSR_SEQ_MAX];
	long used; /* number of path items already accepted */
	grbs_addr_t consider;

	grbs_snapshot_t *snap;
} rbsr_seq_t;


/* Start seqing a routing from tx;ty; returns 0 on success */
int rbsr_seq_begin_at(rbsr_seq_t *rbss, pcb_board_t *pcb, rnd_layer_id_t lid, rnd_coord_t tx, rnd_coord_t ty, rnd_coord_t copper, rnd_coord_t clearance);
void rbsr_seq_end(rbsr_seq_t *rbss);

int rbsr_seq_consider(rbsr_seq_t *rbss, rnd_coord_t tx, rnd_coord_t ty);
void rbsr_seq_accept(rbsr_seq_t *rbss);

