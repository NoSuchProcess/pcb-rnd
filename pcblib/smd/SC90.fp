li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = SC90 SMT transistor, 3 pins
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 0.0
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        12.0mil
        -14.0mil
        -12.0mil
        -14.0mil
        -12.0mil
        14.0mil
        12.0mil
        14.0mil
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
        15.0mil
        -17.0mil
        -15.0mil
        -17.0mil
        -15.0mil
        17.0mil
        15.0mil
        17.0mil
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
        12.0mil
        -14.0mil
        -12.0mil
        -14.0mil
        -12.0mil
        14.0mil
        12.0mil
        14.0mil
       }
       ha:layer_mask {
        top = 1
        paste = 1
       }
       ha:combining {
        auto = 1
       }
      }
     }
     hbottom = 0
     hplated = 0
    }
   }
   li:objects {
    ha:padstack_ref.19 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 30.0mil
     rot = 0.000000
     y = 91.0mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.20 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 0
     xmirror = 0
     x = 69.0mil
     rot = 0.000000
     y = 91.0mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.21 {
     smirror = 0
     ha:attributes {
      term = 3
      name = 3
     }
     proto = 0
     xmirror = 0
     x = 49.0mil
     rot = 0.000000
     y = 32.0mil
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
      ha:line.7 {
       clearance = 0.0
       y2 = 111.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 13.0mil
       x2 = 13.0mil
       ha:flags {
       }
       y1 = 13.0mil
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 111.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 13.0mil
       x2 = 86.0mil
       ha:flags {
       }
       y1 = 111.0mil
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 13.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 86.0mil
       x2 = 86.0mil
       ha:flags {
       }
       y1 = 111.0mil
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 13.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 86.0mil
       x2 = 13.0mil
       ha:flags {
       }
       y1 = 13.0mil
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 106.0mil
       y = 13.0mil
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
      ha:line.22 {
       clearance = 0.0
       y2 = 1.811866mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 1.253066mm
       x2 = 1.253066mm
       ha:flags {
       }
       y1 = 1.811866mm
      }
      ha:line.25 {
       clearance = 0.0
       y2 = 91.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 30.0mil
       x2 = 30.0mil
       ha:flags {
       }
       y1 = 91.0mil
      }
      ha:line.28 {
       clearance = 0.0
       y2 = 3.3114mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 30.0mil
       x2 = 30.0mil
       ha:flags {
       }
       y1 = 91.0mil
      }
      ha:line.31 {
       clearance = 0.0
       y2 = 91.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 30.0mil
       x2 = 1.762mm
       ha:flags {
       }
       y1 = 91.0mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = Y7qfWU1Va5qxQ5ui32QAAAAB
  ha:flags {
  }
 }
}
