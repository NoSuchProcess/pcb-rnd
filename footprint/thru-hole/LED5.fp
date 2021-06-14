li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = LED5, 5mm LED (pin 1 is +, 2 is -)
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 43.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -32.5mil
        -32.5mil
        32.5mil
        -32.5mil
        32.5mil
        32.5mil
        -32.5mil
        32.5mil
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
        -32.5mil
        -32.5mil
        32.5mil
        -32.5mil
        32.5mil
        32.5mil
        -32.5mil
        32.5mil
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
        -32.5mil
        -32.5mil
        32.5mil
        -32.5mil
        32.5mil
        32.5mil
        -32.5mil
        32.5mil
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
        -35.5mil
        -35.5mil
        35.5mil
        -35.5mil
        35.5mil
        35.5mil
        -35.5mil
        35.5mil
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
        -35.5mil
        -35.5mil
        35.5mil
        -35.5mil
        35.5mil
        35.5mil
        -35.5mil
        35.5mil
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
     hdia = 43.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 65.0mil
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
        dia = 65.0mil
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
        dia = 65.0mil
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
        dia = 71.0mil
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
        dia = 71.0mil
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
     x = 2.36474mm
     rot = 0.000000
     y = 3.63474mm
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
     x = 4.90474mm
     rot = 0.000000
     y = 3.63474mm
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
      ha:arc.9 {
       astart = 0
       thickness = 10.0mil
       width = 118.0mil
       height = 118.0mil
       ha:attributes {
       }
       x = 3.63474mm
       y = 3.63474mm
       adelta = 360
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.10 {
       astart = 0
       thickness = 10.0mil
       width = 138.0mil
       height = 138.0mil
       ha:attributes {
       }
       x = 3.63474mm
       y = 3.63474mm
       adelta = 360
       ha:flags {
       }
       clearance = 0.0
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 6.17474mm
       y = 5.41274mm
       rot = 0.000000
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
      ha:line.11 {
       clearance = 0.0
       y2 = 3.63474mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 3.63474mm
       x2 = 3.63474mm
       ha:flags {
       }
       y1 = 3.63474mm
      }
      ha:line.14 {
       clearance = 0.0
       y2 = 3.63474mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 3.63474mm
       x2 = 3.63474mm
       ha:flags {
       }
       y1 = 3.63474mm
      }
      ha:line.17 {
       clearance = 0.0
       y2 = 3.63474mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 3.63474mm
       x2 = 4.63474mm
       ha:flags {
       }
       y1 = 3.63474mm
      }
      ha:line.20 {
       clearance = 0.0
       y2 = 4.63474mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 3.63474mm
       x2 = 3.63474mm
       ha:flags {
       }
       y1 = 3.63474mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = hFGipNdgRBmeMBGUNCcAAAAB
  ha:flags {
  }
 }
}
