li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = TO92 Transistor
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 42.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -36.0mil
        -36.0mil
        36.0mil
        -36.0mil
        36.0mil
        36.0mil
        -36.0mil
        36.0mil
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
        -36.0mil
        -36.0mil
        36.0mil
        -36.0mil
        36.0mil
        36.0mil
        -36.0mil
        36.0mil
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
        -36.0mil
        -36.0mil
        36.0mil
        -36.0mil
        36.0mil
        36.0mil
        -36.0mil
        36.0mil
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
        -39.0mil
        -39.0mil
        39.0mil
        -39.0mil
        39.0mil
        39.0mil
        -39.0mil
        39.0mil
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
        -39.0mil
        -39.0mil
        39.0mil
        -39.0mil
        39.0mil
        39.0mil
        -39.0mil
        39.0mil
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
     hdia = 42.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 72.0mil
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
        dia = 72.0mil
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
        dia = 72.0mil
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
        dia = 78.0mil
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
        dia = 78.0mil
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
     x = 254.0mil
     rot = 0.000000
     y = 200.0mil
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
     x = 154.0mil
     rot = 0.000000
     y = 200.0mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.9 {
     smirror = 0
     ha:attributes {
      term = 3
      name = 3
     }
     proto = 1
     xmirror = 0
     x = 54.0mil
     rot = 0.000000
     y = 200.0mil
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
      ha:line.11 {
       clearance = 0.0
       y2 = 130.0mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 84.0mil
       x2 = 224.0mil
       ha:flags {
       }
       y1 = 130.0mil
      }
      ha:arc.10 {
       astart = 315
       thickness = 10.0mil
       width = 100.0mil
       height = 100.0mil
       ha:attributes {
       }
       x = 154.0mil
       y = 200.0mil
       adelta = 270
       ha:flags {
       }
       clearance = 0.0
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 64.0mil
       y = 70.0mil
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
      ha:line.14 {
       clearance = 0.0
       y2 = 200.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 154.0mil
       x2 = 154.0mil
       ha:flags {
       }
       y1 = 200.0mil
      }
      ha:line.17 {
       clearance = 0.0
       y2 = 200.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 54.0mil
       x2 = 54.0mil
       ha:flags {
       }
       y1 = 200.0mil
      }
      ha:line.20 {
       clearance = 0.0
       y2 = 4.08mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 54.0mil
       x2 = 54.0mil
       ha:flags {
       }
       y1 = 200.0mil
      }
      ha:line.23 {
       clearance = 0.0
       y2 = 200.0mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 54.0mil
       x2 = 0.3716mm
       ha:flags {
       }
       y1 = 200.0mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = guzk2KSYW/BVDIzhJVkAAAAB
  ha:flags {
  }
 }
}
