li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint =  OSC14, Crystal oscillator
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 28.0mil
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 50.0mil
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
        dia = 50.0mil
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
        dia = 50.0mil
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
        dia = 56.0mil
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
        dia = 56.0mil
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
      name = NC
     }
     proto = 0
     xmirror = 0
     x = 2.54254mm
     rot = 0.000000
     y = 2.54254mm
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
      name = GND
     }
     proto = 0
     xmirror = 0
     x = 2.54254mm
     rot = 0.000000
     y = 700.1mil
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
      name = CLK
     }
     proto = 0
     xmirror = 0
     x = 400.1mil
     rot = 0.000000
     y = 700.1mil
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.10 {
     smirror = 0
     ha:attributes {
      term = 4
      name = VCC
     }
     proto = 0
     xmirror = 0
     x = 400.1mil
     rot = 0.000000
     y = 2.54254mm
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
       y2 = 0.12954mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.12954mm
       x2 = 400.1mil
       ha:flags {
       }
       y1 = 0.12954mm
      }
      ha:line.15 {
       clearance = 0.0
       y2 = 700.1mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 495.1mil
       x2 = 495.1mil
       ha:flags {
       }
       y1 = 2.54254mm
      }
      ha:line.19 {
       clearance = 0.0
       y2 = 795.1mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 400.1mil
       x2 = 2.54254mm
       ha:flags {
       }
       y1 = 795.1mil
      }
      ha:line.23 {
       clearance = 0.0
       y2 = 0.12954mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.12954mm
       x2 = 0.12954mm
       ha:flags {
       }
       y1 = 700.1mil
      }
      ha:line.26 {
       clearance = 0.0
       y2 = 60.1mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 2.54254mm
       x2 = 400.1mil
       ha:flags {
       }
       y1 = 60.1mil
      }
      ha:line.30 {
       clearance = 0.0
       y2 = 700.1mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 440.1mil
       x2 = 440.1mil
       ha:flags {
       }
       y1 = 2.54254mm
      }
      ha:line.34 {
       clearance = 0.0
       y2 = 740.1mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 400.1mil
       x2 = 2.54254mm
       ha:flags {
       }
       y1 = 740.1mil
      }
      ha:line.38 {
       clearance = 0.0
       y2 = 2.54254mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 60.1mil
       x2 = 60.1mil
       ha:flags {
       }
       y1 = 700.1mil
      }
      ha:arc.14 {
       astart = 180
       thickness = 10.0mil
       width = 95.0mil
       height = 95.0mil
       ha:attributes {
       }
       x = 400.1mil
       y = 2.54254mm
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.18 {
       astart = 90
       thickness = 10.0mil
       width = 95.0mil
       height = 95.0mil
       ha:attributes {
       }
       x = 400.1mil
       y = 700.1mil
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.22 {
       astart = 0
       thickness = 10.0mil
       width = 95.0mil
       height = 95.0mil
       ha:attributes {
       }
       x = 2.54254mm
       y = 700.1mil
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.29 {
       astart = 180
       thickness = 10.0mil
       width = 40.0mil
       height = 40.0mil
       ha:attributes {
       }
       x = 400.1mil
       y = 2.54254mm
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.33 {
       astart = 90
       thickness = 10.0mil
       width = 40.0mil
       height = 40.0mil
       ha:attributes {
       }
       x = 400.1mil
       y = 700.1mil
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.37 {
       astart = 0
       thickness = 10.0mil
       width = 40.0mil
       height = 40.0mil
       ha:attributes {
       }
       x = 2.54254mm
       y = 700.1mil
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:arc.41 {
       astart = 270
       thickness = 10.0mil
       width = 40.0mil
       height = 40.0mil
       ha:attributes {
       }
       x = 2.54254mm
       y = 2.54254mm
       adelta = 90
       ha:flags {
       }
       clearance = 0.0
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 270.1mil
       y = 300.1mil
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
      ha:line.42 {
       clearance = 0.0
       y2 = 400.1mil
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 6.35254mm
       x2 = 6.35254mm
       ha:flags {
       }
       y1 = 400.1mil
      }
      ha:line.45 {
       clearance = 0.0
       y2 = 2.54254mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 2.54254mm
       x2 = 2.54254mm
       ha:flags {
       }
       y1 = 2.54254mm
      }
      ha:line.48 {
       clearance = 0.0
       y2 = 2.54254mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 2.54254mm
       x2 = 3.54254mm
       ha:flags {
       }
       y1 = 2.54254mm
      }
      ha:line.51 {
       clearance = 0.0
       y2 = 3.54254mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 2.54254mm
       x2 = 2.54254mm
       ha:flags {
       }
       y1 = 2.54254mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = xzk533cq9lHncC5p9N4AAAAB
  ha:flags {
  }
 }
}
