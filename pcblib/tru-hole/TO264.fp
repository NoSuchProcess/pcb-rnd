li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = TO264 diode in TO220
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 60.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       li:ps_poly {
        -50.0mil
        -50.0mil
        50.0mil
        -50.0mil
        50.0mil
        50.0mil
        -50.0mil
        50.0mil
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
        -50.0mil
        -50.0mil
        50.0mil
        -50.0mil
        50.0mil
        50.0mil
        -50.0mil
        50.0mil
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
        -50.0mil
        -50.0mil
        50.0mil
        -50.0mil
        50.0mil
        50.0mil
        -50.0mil
        50.0mil
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
        -53.0mil
        -53.0mil
        53.0mil
        -53.0mil
        53.0mil
        53.0mil
        -53.0mil
        53.0mil
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
        -53.0mil
        -53.0mil
        53.0mil
        -53.0mil
        53.0mil
        53.0mil
        -53.0mil
        53.0mil
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
     hdia = 60.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 100.0mil
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
        dia = 100.0mil
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
        dia = 100.0mil
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
        dia = 106.0mil
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
        dia = 106.0mil
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
     x = 4.852183mm
     rot = 0.000000
     y = 3.556783mm
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
     x = 10.414783mm
     rot = 0.000000
     y = 3.556783mm
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
     x = 15.977383mm
     rot = 0.000000
     y = 3.556783mm
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
       y2 = 5.588783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 5.588783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 20.574783mm
       ha:flags {
       }
       y1 = 5.588783mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 20.574783mm
       x2 = 20.574783mm
       ha:flags {
       }
       y1 = 5.588783mm
      }
      ha:line.19 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 20.574783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.22 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 20.574783mm
       ha:flags {
       }
       y1 = 1.524783mm
      }
      ha:line.25 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 8.509783mm
       x2 = 8.509783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.28 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 12.319783mm
       x2 = 12.319783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 22.098783mm
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
      ha:line.31 {
       clearance = 0.0
       y2 = 3.556783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 10.414783mm
       x2 = 10.414783mm
       ha:flags {
       }
       y1 = 3.556783mm
      }
      ha:line.34 {
       clearance = 0.0
       y2 = 7.112783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 4.852183mm
       x2 = 4.852183mm
       ha:flags {
       }
       y1 = 7.112783mm
      }
      ha:line.37 {
       clearance = 0.0
       y2 = 8.112783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 4.852183mm
       x2 = 4.852183mm
       ha:flags {
       }
       y1 = 7.112783mm
      }
      ha:line.40 {
       clearance = 0.0
       y2 = 7.112783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 4.852183mm
       x2 = 5.852183mm
       ha:flags {
       }
       y1 = 7.112783mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = 8OAC9HM9Aw1ZzceKxDMAAAAB
  ha:flags {
  }
 }
}
