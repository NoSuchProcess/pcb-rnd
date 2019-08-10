li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = LED3, 3mm LED (pin 1 is +, 2 is -)
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
     x = 50.5mil
     rot = 0.000000
     y = 2.13614mm
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
     x = 150.5mil
     rot = 0.000000
     y = 2.13614mm
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
       astart = 45
       thickness = 10.0mil
       width = 59.0mil
       height = 59.0mil
       ha:attributes {
       }
       x = 100.5mil
       y = 2.13614mm
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.10 {
       astart = 225
       thickness = 10.0mil
       width = 59.0mil
       height = 59.0mil
       ha:attributes {
       }
       x = 100.5mil
       y = 2.13614mm
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.11 {
       astart = 45
       thickness = 10.0mil
       width = 79.0mil
       height = 79.0mil
       ha:attributes {
       }
       x = 100.5mil
       y = 2.13614mm
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.12 {
       astart = 225
       thickness = 10.0mil
       width = 79.0mil
       height = 79.0mil
       ha:attributes {
       }
       x = 100.5mil
       y = 2.13614mm
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 200.5mil
       y = 3.91414mm
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
      ha:line.13 {
       clearance = 0.0
       y2 = 2.13614mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 100.5mil
       x2 = 100.5mil
       ha:flags {
       }
       y1 = 2.13614mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 2.13614mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 100.5mil
       x2 = 100.5mil
       ha:flags {
       }
       y1 = 2.13614mm
      }
      ha:line.19 {
       clearance = 0.0
       y2 = 2.13614mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 100.5mil
       x2 = 3.5527mm
       ha:flags {
       }
       y1 = 2.13614mm
      }
      ha:line.22 {
       clearance = 0.0
       y2 = 3.13614mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 100.5mil
       x2 = 100.5mil
       ha:flags {
       }
       y1 = 2.13614mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = AXOBLf2n2pEOmUoe7pcAAAAB
  ha:flags {
  }
 }
}
