li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   value =
   footprint = HC49U_3 Crystals
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 32.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
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
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
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
        -30.0mil
        -30.0mil
        30.0mil
        -30.0mil
        30.0mil
        30.0mil
        -30.0mil
        30.0mil
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
        -33.0mil
        -33.0mil
        33.0mil
        -33.0mil
        33.0mil
        33.0mil
        -33.0mil
        33.0mil
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
        -33.0mil
        -33.0mil
        33.0mil
        -33.0mil
        33.0mil
        33.0mil
        -33.0mil
        33.0mil
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
     hdia = 32.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 60.0mil
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
        dia = 60.0mil
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
        dia = 60.0mil
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
        dia = 66.0mil
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
        dia = 66.0mil
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
     x = 3.33248mm
     rot = 0.000000
     y = 101.2mil
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
     x = 5.77088mm
     rot = 0.000000
     y = 101.2mil
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
     x = 8.20928mm
     rot = 0.000000
     y = 101.2mil
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
      ha:line.10 {
       clearance = 0.0
       y2 = 0.25908mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 101.2mil
       x2 = 8.99668mm
       ha:flags {
       }
       y1 = 0.25908mm
      }
      ha:line.14 {
       clearance = 0.0
       y2 = 4.90728mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 8.99668mm
       x2 = 101.2mil
       ha:flags {
       }
       y1 = 4.90728mm
      }
      ha:arc.13 {
       astart = 90
       thickness = 20.0mil
       width = 91.0mil
       height = 91.0mil
       ha:attributes {
       }
       x = 8.99668mm
       y = 101.2mil
       adelta = 180
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.17 {
       astart = 270
       thickness = 20.0mil
       width = 91.0mil
       height = 91.0mil
       ha:attributes {
       }
       x = 101.2mil
       y = 101.2mil
       adelta = 180
       ha:flags {
       }
       clearance = 0.0
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 0.25908mm
       y = -1.26492mm
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
      ha:line.18 {
       clearance = 0.0
       y2 = 101.2mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 5.77088mm
       x2 = 5.77088mm
       ha:flags {
       }
       y1 = 101.2mil
      }
      ha:line.21 {
       clearance = 0.0
       y2 = 101.2mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 3.33248mm
       x2 = 3.33248mm
       ha:flags {
       }
       y1 = 101.2mil
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 3.57048mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 3.33248mm
       x2 = 3.33248mm
       ha:flags {
       }
       y1 = 101.2mil
      }
      ha:line.27 {
       clearance = 0.0
       y2 = 101.2mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 3.33248mm
       x2 = 4.33248mm
       ha:flags {
       }
       y1 = 101.2mil
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = yz1V4HtC1LifNPNIfzcAAAAB
  ha:flags {
  }
 }
}
