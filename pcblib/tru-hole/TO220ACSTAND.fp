li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = TO220ACSTAND diode in TO220
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 40.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -40.0mil
        -40.0mil
        40.0mil
        -40.0mil
        40.0mil
        40.0mil
        -40.0mil
        40.0mil
       }
       ha:layer_mask {
        copper = 1
        top = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -40.0mil
        -40.0mil
        40.0mil
        -40.0mil
        40.0mil
        40.0mil
        -40.0mil
        40.0mil
       }
       ha:layer_mask {
        bottom = 1
        copper = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -40.0mil
        -40.0mil
        40.0mil
        -40.0mil
        40.0mil
        40.0mil
        -40.0mil
        40.0mil
       }
       ha:layer_mask {
        copper = 1
        intern = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -43.0mil
        -43.0mil
        43.0mil
        -43.0mil
        43.0mil
        43.0mil
        -43.0mil
        43.0mil
       }
       ha:layer_mask {
        top = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -43.0mil
        -43.0mil
        43.0mil
        -43.0mil
        43.0mil
        43.0mil
        -43.0mil
        43.0mil
       }
       ha:layer_mask {
        bottom = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 1
    }
    ha:ps_proto_v6.1 {
     htop = 0
     hdia = 40.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 80.0mil
       }
       ha:layer_mask {
        copper = 1
        top = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 80.0mil
       }
       ha:layer_mask {
        bottom = 1
        copper = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 80.0mil
       }
       ha:layer_mask {
        copper = 1
        intern = 1
       }
       ha:combining {
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 86.0mil
       }
       ha:layer_mask {
        top = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 86.0mil
       }
       ha:layer_mask {
        bottom = 1
        mask = 1
       }
       ha:combining {
        sub = 1
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 1
    }
   }
   li:objects {
    ha:padstack_ref.7 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 2.794783mm
     rot = 0.000000
     y = 2.794783mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.8 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 1
     xmirror = 0
     x = 7.874783mm
     rot = 0.000000
     y = 2.794783mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
   }
   li:layers {
    ha:top-silk {
     lid = 0
     ha:type {
      silk = 1
      top = 1
     }
     li:objects {
      ha:line.9 {
       clearance = 0.0
       y2 = 4.826783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.12 {
       clearance = 0.0
       y2 = 4.826783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 10.414783mm
       ha:flags {
       }
       y1 = 4.826783mm
      }
      ha:line.15 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 10.414783mm
       x2 = 10.414783mm
       ha:flags {
       }
       y1 = 4.826783mm
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 10.414783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.21 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 10.414783mm
       ha:flags {
       }
       y1 = 1.524783mm
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 3.429783mm
       x2 = 3.429783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.27 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 7.239783mm
       x2 = 7.239783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 11.938783mm
       y = 1.524783mm
       rot = 270.000000
       string = %a.parent.refdes%
       fid = 0
       ha:flags {
        dyntext = 1
        floater = 1
       }
      }
     }
     ha:combining {
     }
    }
    ha:subc-aux {
     lid = 1
     ha:type {
      top = 1
      misc = 1
      virtual = 1
     }
     li:objects {
      ha:line.30 {
       clearance = 0.0
       y2 = 2.794783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 5.334783mm
       x2 = 5.334783mm
       ha:flags {
       }
       y1 = 2.794783mm
      }
      ha:line.33 {
       clearance = 0.0
       y2 = 5.842783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 2.794783mm
       x2 = 2.794783mm
       ha:flags {
       }
       y1 = 5.842783mm
      }
      ha:line.36 {
       clearance = 0.0
       y2 = 5.842783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 2.794783mm
       x2 = 3.794783mm
       ha:flags {
       }
       y1 = 5.842783mm
      }
      ha:line.39 {
       clearance = 0.0
       y2 = 6.842783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 2.794783mm
       x2 = 2.794783mm
       ha:flags {
       }
       y1 = 5.842783mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = giMFC8I9a3NsCRqIha4AAAAB
  ha:flags {
  }
 }
}
