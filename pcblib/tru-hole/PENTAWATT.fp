li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = PENTAWATT Power IC
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
        -45.0mil
        -45.0mil
        45.0mil
        -45.0mil
        45.0mil
        45.0mil
        -45.0mil
        45.0mil
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
        -45.0mil
        -45.0mil
        45.0mil
        -45.0mil
        45.0mil
        45.0mil
        -45.0mil
        45.0mil
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
        -45.0mil
        -45.0mil
        45.0mil
        -45.0mil
        45.0mil
        45.0mil
        -45.0mil
        45.0mil
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
        -48.0mil
        -48.0mil
        48.0mil
        -48.0mil
        48.0mil
        48.0mil
        -48.0mil
        48.0mil
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
        -48.0mil
        -48.0mil
        48.0mil
        -48.0mil
        48.0mil
        48.0mil
        -48.0mil
        48.0mil
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
        dia = 90.0mil
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
        dia = 90.0mil
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
        dia = 90.0mil
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
        dia = 96.0mil
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
        dia = 96.0mil
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
     x = 2.032783mm
     rot = 0.000000
     y = 8.738383mm
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
     x = 3.734583mm
     rot = 0.000000
     y = 4.750583mm
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
     x = 5.436383mm
     rot = 0.000000
     y = 8.738383mm
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
      name = 4
     }
     proto = 1
     xmirror = 0
     x = 7.138183mm
     rot = 0.000000
     y = 4.750583mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 15.0mil
    }
    ha:padstack_ref.11 {
     smirror = 0
     ha:attributes {
      term = 5
      name = 5
     }
     proto = 1
     xmirror = 0
     x = 8.839983mm
     rot = 0.000000
     y = 8.738383mm
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
      ha:line.12 {
       clearance = 0.0
       y2 = 5.055383mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.15 {
       clearance = 0.0
       y2 = 5.055383mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 10.643383mm
       ha:flags {
       }
       y1 = 5.055383mm
      }
      ha:line.18 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 10.643383mm
       x2 = 10.643383mm
       ha:flags {
       }
       y1 = 5.055383mm
      }
      ha:line.21 {
       clearance = 0.0
       y2 = 0.254783mm
       thickness = 20.0mil
       ha:attributes {
       }
       x1 = 10.643383mm
       x2 = 0.254783mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.24 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 0.254783mm
       x2 = 10.643383mm
       ha:flags {
       }
       y1 = 1.524783mm
      }
      ha:line.27 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 3.531383mm
       x2 = 3.531383mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:line.30 {
       clearance = 0.0
       y2 = 1.524783mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 7.341383mm
       x2 = 7.341383mm
       ha:flags {
       }
       y1 = 0.254783mm
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 12.167383mm
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
      ha:line.33 {
       clearance = 0.0
       y2 = 7.143263mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 5.436383mm
       x2 = 5.436383mm
       ha:flags {
       }
       y1 = 7.143263mm
      }
      ha:line.36 {
       clearance = 0.0
       y2 = 6.579383mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 2.032783mm
       x2 = 2.032783mm
       ha:flags {
       }
       y1 = 6.579383mm
      }
      ha:line.39 {
       clearance = 0.0
       y2 = 7.579383mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 2.032783mm
       x2 = 2.032783mm
       ha:flags {
       }
       y1 = 6.579383mm
      }
      ha:line.42 {
       clearance = 0.0
       y2 = 6.579383mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 2.032783mm
       x2 = 3.032783mm
       ha:flags {
       }
       y1 = 6.579383mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = sniNjrnRQefHo+GXiZsAAAAB
  ha:flags {
  }
 }
}
