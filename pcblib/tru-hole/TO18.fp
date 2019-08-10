li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = TO18 Transistor
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 35.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 55.0mil
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
        dia = 55.0mil
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
        dia = 55.0mil
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
        dia = 61.0mil
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
        dia = 61.0mil
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
     x = 2.61874mm
     rot = 0.000000
     y = 1.549792mm
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
     proto = 0
     xmirror = 0
     x = 53.1mil
     rot = 0.000000
     y = 2.819792mm
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
     proto = 0
     xmirror = 0
     x = 2.61874mm
     rot = 0.000000
     y = 4.089792mm
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
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 4.32054mm
       x2 = 5.00634mm
       ha:flags {
       }
       y1 = 0.813192mm
      }
      ha:line.14 {
       clearance = 0.0
       y2 = 0.279792mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 4.47294mm
       x2 = 5.15874mm
       ha:flags {
       }
       y1 = 0.965592mm
      }
      ha:line.17 {
       clearance = 0.0
       y2 = 0.432192mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 4.62534mm
       x2 = 5.31114mm
       ha:flags {
       }
       y1 = 1.117992mm
      }
      ha:line.20 {
       clearance = 0.0
       y2 = 0.432192mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 5.00634mm
       x2 = 5.31114mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:arc.10 {
       astart = 0
       thickness = 10.0mil
       width = 98.0mil
       height = 98.0mil
       ha:attributes {
       }
       x = 2.61874mm
       y = 2.819792mm
       adelta = 360
       ha:flags {
       }
       clearance = 0.0
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 4.14274mm
       y = 4.597792mm
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
      ha:line.23 {
       clearance = 0.0
       y2 = 2.819792mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 2.195406mm
       x2 = 2.195406mm
       ha:flags {
       }
       y1 = 2.819792mm
      }
      ha:line.26 {
       clearance = 0.0
       y2 = 2.819792mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 2.61874mm
       x2 = 2.61874mm
       ha:flags {
       }
       y1 = 2.819792mm
      }
      ha:line.29 {
       clearance = 0.0
       y2 = 1.819792mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 2.61874mm
       x2 = 2.61874mm
       ha:flags {
       }
       y1 = 2.819792mm
      }
      ha:line.32 {
       clearance = 0.0
       y2 = 2.819792mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 2.61874mm
       x2 = 1.61874mm
       ha:flags {
       }
       y1 = 2.819792mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = OidYcSm6o+LolzS8Q8kAAAAB
  ha:flags {
  }
 }
}
