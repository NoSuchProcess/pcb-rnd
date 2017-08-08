#warning TODO: move this back to undo_old.c

typedef struct {								/* information about a change command */
	char *Name;
} ChangeNameType, *ChangeNameTypePtr;

typedef struct {								/* information about a move command */
	pcb_coord_t DX, DY;									/* movement vector */
} MoveType, *MoveTypePtr;

typedef struct {								/* information about removed polygon points */
	pcb_coord_t X, Y;										/* data */
	int ID;
	pcb_cardinal_t Index;								/* index in a polygons array of points */
	pcb_bool last_in_contour;					/* Whether the point was the last in its contour */
} RemovedPointType, *RemovedPointTypePtr;

typedef struct {								/* information about rotation */
	pcb_coord_t CenterX, CenterY;				/* center of rotation */
	pcb_cardinal_t Steps;								/* number of steps */
} RotateType, *RotateTypePtr;

typedef struct {								/* information about moves between layers */
	pcb_cardinal_t OriginalLayer;				/* the index of the original layer */
} MoveToLayer;

typedef struct {								/* information about layer changes */
	int old_index;
	int new_index;
} LayerChangeType, *LayerChangeTypePtr;

typedef struct {								/* information about poly clear/restore */
	pcb_bool Clear;										/* pcb_true was clear, pcb_false was restore */
	pcb_layer_t *Layer;
} ClearPolyType, *ClearPolyTypePtr;

typedef struct {
	pcb_angle_t angle[2];
} AngleChangeType;

typedef struct {								/* information about netlist lib changes */
	pcb_lib_t *old;
	pcb_lib_t *lib;
} NetlistChangeType, *NetlistChangeTypePtr;

typedef struct {								/* holds information about an operation */
	int Serial,										/* serial number of operation */
	  Type,												/* type of operation */
	  Kind,												/* type of object with given ID */
	  ID;													/* object ID */
	union {												/* some additional information */
		ChangeNameType ChangeName;
		MoveType Move;
		RemovedPointType RemovedPoint;
		RotateType Rotate;
		MoveToLayer MoveToLayer;
		pcb_flag_t Flags;
		pcb_coord_t Size;
		LayerChangeType LayerChange;
		ClearPolyType ClearPoly;
		NetlistChangeType NetlistChange;
		long int CopyID;
		AngleChangeType AngleChange;
	} Data;
} UndoListType, *UndoListTypePtr;
