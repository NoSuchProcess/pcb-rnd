li:pcb-rnd-subcircuit-v6 {
 ha:subc.5 {
  ha:attributes {
   footprint = power jack
  }
  ha:data {
   li:padstack_prototypes {
    ha:ps_proto_v6.0 {
     htop = 0
     hdia = 2.999994mm
     li:shape {
      ha:ps_shape_v4 {
       clearance = 0.0
       ha:ps_circ {
        x = 0.0
        y = 0.0
        dia = 4.99999mm
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
        dia = 4.99999mm
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
        dia = 4.99999mm
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
        dia = 5.15239mm
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
        dia = 5.15239mm
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
      term = 2
     }
     proto = 0
     xmirror = 0
     x = 1.24016in
     rot = 0.000000
     y = 35.999928mm
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
    }
    ha:padstack_ref.8 {
     smirror = 0
     ha:attributes {
      term = 1
     }
     proto = 0
     xmirror = 0
     x = 1.24016in
     rot = 0.000000
     y = 1.14173in
     li:thermal {
     }
     ha:flags {
      clearline = 1
     }
     clearance = 10.0mil
    }
    ha:padstack_ref.9 {
     smirror = 0
     ha:attributes {
      term = 3
     }
     proto = 0
     xmirror = 0
     x = 1.06299in
     rot = 0.000000
     y = 1.27953in
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
      ha:line.10 {
       clearance = 0.0
       y2 = 35.999928mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 1.06299in
       x2 = 1.06299in
       ha:flags {
       }
       y1 = 21.999956mm
      }
      ha:line.13 {
       clearance = 0.0
       y2 = 35.999928mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 1.06299in
       x2 = 35.999928mm
       ha:flags {
       }
       y1 = 35.999928mm
      }
      ha:line.16 {
       clearance = 0.0
       y2 = 21.999956mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 35.999928mm
       x2 = 35.999928mm
       ha:flags {
       }
       y1 = 35.999928mm
      }
      ha:line.19 {
       clearance = 0.0
       y2 = 21.999956mm
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 35.999928mm
       x2 = 1.06299in
       ha:flags {
       }
       y1 = 21.999956mm
      }
      ha:line.22 {
       clearance = 0.0
       y2 = 984.25mil
       thickness = 10.0mil
       ha:attributes {
       }
       x1 = 1.06299in
       x2 = 35.999928mm
       ha:flags {
       }
       y1 = 984.25mil
      }
      ha:text.6 {
       scale = 100
       ha:attributes {
       }
       x = 1.24016in
       y = 35.999928mm
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
      ha:line.25 {
       clearance = 0.0
       y2 = 32.499977mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = pnp-origin
       }
       x1 = 1.18110331in
       x2 = 1.18110331in
       ha:flags {
       }
       y1 = 32.499977mm
      }
      ha:line.28 {
       clearance = 0.0
       y2 = 35.999928mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = origin
       }
       x1 = 1.24016in
       x2 = 1.24016in
       ha:flags {
       }
       y1 = 35.999928mm
      }
      ha:line.31 {
       clearance = 0.0
       y2 = 34.999928mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = x
       }
       x1 = 1.24016in
       x2 = 1.24016in
       ha:flags {
       }
       y1 = 35.999928mm
      }
      ha:line.34 {
       clearance = 0.0
       y2 = 35.999928mm
       thickness = 0.1mm
       ha:attributes {
        subc-role = y
       }
       x1 = 1.24016in
       x2 = 30.500064mm
       ha:flags {
       }
       y1 = 35.999928mm
      }
     }
     ha:combining {
     }
    }
   }
  }
  uid = N9dhMGRbIqPCtXUXD0MAAAAB
  ha:flags {
  }
 }
}
