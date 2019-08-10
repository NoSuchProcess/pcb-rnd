li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = TO247_2 diode in TO220
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
     x = 2.693183mm
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
     x = 13.818383mm
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
      ha:line.9 {
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
      ha:line.12 {
       clearance = 0.0
       y2 = 5.588783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 16.256783mm
       ha:flags {
       }
       y1 = 5.588783mm
      }
      ha:line.15 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 16.256783mm
       x2 = 16.256783mm
       ha:flags {
       }
       y1 = 5.588783mm
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 16.256783mm
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
       x2 = 16.256783mm
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
       x1 = 6.350783mm
       x2 = 6.350783mm
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
       x1 = 10.160783mm
       x2 = 10.160783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 17.780783mm
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
       y2 = 3.556783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 8.255783mm
       x2 = 8.255783mm
       ha:flags {
       }
       y1 = 3.556783mm
      }
      ha:line.33 {
       clearance = 0.0
       y2 = 7.112783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 2.693183mm
       x2 = 2.693183mm
       ha:flags {
       }
       y1 = 7.112783mm
      }
      ha:line.36 {
       clearance = 0.0
       y2 = 7.112783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 2.693183mm
       x2 = 3.693183mm
       ha:flags {
       }
       y1 = 7.112783mm
      }
      ha:line.39 {
       clearance = 0.0
       y2 = 8.112783mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 2.693183mm
       x2 = 2.693183mm
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
  uid = WJYlfial3zliAL7vW/EAAAAB
  ha:flags {
  }
 }
}
