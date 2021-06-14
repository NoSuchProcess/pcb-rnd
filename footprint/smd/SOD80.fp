li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = SOD80 SMT diode (pin 1 is cathode)
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
        26.5mil
        -42.5mil
        -26.5mil
        -42.5mil
        -26.5mil
        42.5mil
        26.5mil
        42.5mil
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
        29.5mil
        -45.5mil
        -29.5mil
        -45.5mil
        -29.5mil
        45.5mil
        29.5mil
        45.5mil
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
        26.5mil
        -42.5mil
        -26.5mil
        -42.5mil
        -26.5mil
        42.5mil
        26.5mil
        42.5mil
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
    ha:padstack_ref.25 {
     smirror = 0
     ha:attributes {
      term = 1
      name = 1
     }
     proto = 0
     xmirror = 0
     x = 1.473983mm
     rot = 0.000000
     y = 1.524392mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
    }
    ha:padstack_ref.26 {
     smirror = 0
     ha:attributes {
      term = 2
      name = 2
     }
     proto = 0
     xmirror = 0
     x = 4.420383mm
     rot = 0.000000
     y = 1.524392mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
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
       y2 = 2.616592mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.432192mm
      }
      ha:line.10 {
       clearance = 0.0
       y2 = 2.921392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 0.915183mm
       ha:flags {
       }
       y1 = 2.616592mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 2.921392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.915183mm
       x2 = 5.385583mm
       ha:flags {
       }
       y1 = 2.921392mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 5.385583mm
       x2 = 5.385583mm
       ha:flags {
       }
       y1 = 2.921392mm
      }
      ha:line.19 {
       clearance = 0.0
       y2 = 0.127392mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 5.385583mm
       x2 = 0.915183mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:line.22 {
       clearance = 0.0
       y2 = 0.432192mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.915183mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.127392mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 5.893583mm
       y = 1.524392mm
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
      ha:line.27 {
       clearance = 0.0
       y2 = 1.524392mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 2.947183mm
       x2 = 2.947183mm
       ha:flags {
       }
       y1 = 1.524392mm
      }
      ha:line.30 {
       clearance = 0.0
       y2 = 1.524392mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 2.947183mm
       x2 = 2.947183mm
       ha:flags {
       }
       y1 = 1.524392mm
      }
      ha:line.33 {
       clearance = 0.0
       y2 = 1.524392mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 2.947183mm
       x2 = 3.947183mm
       ha:flags {
       }
       y1 = 1.524392mm
      }
      ha:line.36 {
       clearance = 0.0
       y2 = 2.524392mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 2.947183mm
       x2 = 2.947183mm
       ha:flags {
       }
       y1 = 1.524392mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = VTNdQbVwPxfgsvfI80oAAAAB
  ha:flags {
  }
 }
}
