/*
 * usb_pd.h
 *
 *  Created on: Feb 9, 2026
 *      Author: xiongwei
 */

#ifndef CORE_USB_PD_H_
#define CORE_USB_PD_H_


// Power Data Object (PDO) (32-bit descriptor)
// common
#define USB_PDO_TYPE_MASK                 0x03
#define USB_PDO_TYPE_SHIFT                30
#define USB_PDO_TYPE_FIXED                (0 << USB_PDO_TYPE_SHIFT)
#define USB_PDO_TYPE_BATTERY              (1 << USB_PDO_TYPE_SHIFT)
#define USB_PDO_TYPE_VARIABLE             (2 << USB_PDO_TYPE_SHIFT)

// Fixed Supply PDO
// define USB_PDO_TYPE_FIXED
//  | USB_PDO_DUAL_ROLE | USB_PDO_USB_SUSPEND
//  | USB_PDO_USB_CAPABLE | USB_PDO_DATA_ROLE_SWAP
//  | USB_PDO_VOLTAGE(x) | USB_PDO_CURRENT(x)
// for Orion, define USB_PDO_TYPE_FIXED
//  | USB_PDO_DUAL_ROLE
//  | USB_PDO_VOLTAGE(x) | USB_PDO_CURRENT(x)
#define USB_PDO_DUAL_ROLE_SHIFT           29
#define USB_PDO_DUAL_ROLE                 (1 << USB_PDO_DUAL_ROLE_SHIFT)

#define USB_PDO_USB_SUSPEND_SHIFT         28
#define USB_PDO_USB_SUSPEND               (1 << USB_PDO_USB_SUSPEND_SHIFT)

#define USB_PDO_EXTERNAL_POWER_SHIFT      27
#define USB_PDO_EXTERNAL_POWER            (1 << USB_PDO_EXTERNAL_POWER_SHIFT)

#define USB_PDO_USB_CAPABLE_SHIFT         26
#define USB_PDO_USB_CAPABLE               (1 << USB_PDO_USB_CAPABLE_SHIFT)

#define USB_PDO_DATA_ROLE_SWAP_SHIFT      25
#define USB_PDO_DATA_ROLE_SWAP            (1 << USB_PDO_DATA_ROLE_SWAP_SHIFT)

#define USB_PDO_PEAK_CURRENT_SHIFT        20
#define USB_PDO_PEAK_CURRENT_MASK         0x03
#define USB_PDO_PEAK_CURRENT_NONE         (0 << USB_PDO_PEAK_CURRENT_SHIFT)
#define USB_PDO_PEAK_CURRENT_150PCT       (1 << USB_PDO_PEAK_CURRENT_SHIFT)
#define USB_PDO_PEAK_CURRENT_200PCT       (2 << USB_PDO_PEAK_CURRENT_SHIFT)
#define USB_PDO_PEAK_CURRENT_200PCT_PLUS  (3 << USB_PDO_PEAK_CURRENT_SHIFT)

#define USB_PDO_VOLTAGE_SHIFT             10
#define USB_PDO_VOLTAGE_MASK              ((1 << (USB_PDO_PEAK_CURRENT_SHIFT - USB_PDO_VOLTAGE_SHIFT)) - 1)
#define USB_PDO_VOLTAGE(x)                ((((x) / 50) & USB_PDO_VOLTAGE_MASK) << USB_PDO_VOLTAGE_SHIFT)
                                            // x in mV

#define USB_PDO_CURRENT_SHIFT             0
#define USB_PDO_CURRENT_MASK              ((1 << (USB_PDO_VOLTAGE_SHIFT - USB_PDO_CURRENT_SHIFT)) - 1)
#define USB_PDO_CURRENT(x)                ((((x) / 10) & USB_PDO_CURRENT_MASK) << USB_PDO_CURRENT_SHIFT)
                                            // x in ma


// Variable Supply PDO
// define USB_PDO_TYPE_VARIABLE | USB_PDO_MAX_VOLTAGE(x) | USB_PDO_MIN_VOLTAGE(x) | USB_PDO_CURRENT(x)
#define USB_PDO_MAX_VOLTAGE_SHIFT         20
#define USB_PDO_MAX_VOLTAGE_MASK          ((1 << (USB_PDO_TYPE_SHIFT - USB_PDO_MAX_VOLTAGE_SHIFT)) - 1)
#define USB_PDO_MAX_VOLTAGE(x)            ((((x) / 50) & USB_PDO_MAX_VOLTAGE_MASK) << USB_PDO_MAX_VOLTAGE_SHIFT)
                                            // x in mV

#define USB_PDO_MIN_VOLTAGE_SHIFT         10
#define USB_PDO_MIN_VOLTAGE_MASK          ((1 << (USB_PDO_MAX_VOLTAGE_SHIFT - USB_PDO_MIN_VOLTAGE_SHIFT)) - 1)
#define USB_PDO_MIN_VOLTAGE(x)                ((((x) / 50) & USB_PDO_VOLTAGE_MASK) << USB_PDO_VOLTAGE_SHIFT)
                                            // x in mV


// Battery Supply PDO
// define USB_PDO_TYPE_BATTERY | USB_PDO_MAX_VOLTAGE(x) | USB_PDO_MIN_VOLTAGE(x) | USB_PDO_POWER(x)
#define USB_PDO_POWER_SHIFT               0
#define USB_PDO_POWER_MASK                ((1 << (USB_PDO_VOLTAGE_SHIFT - USB_PDO_CURRENT_SHIFT)) - 1)
#define USB_PDO_POWER(x)                  ((((x) / 250) & USB_PDO_CURRENT_MASK) << USB_PDO_CURRENT_SHIFT)
                                            // x in mW


// Fixed Sink PDO
// define USB_PDO_TYPE_FIXED
//  | USB_PDO_DUAL_ROLE | USB_PDO_HIGH_VOLTAGE
//  | USB_PDO_USB_CAPABLE | USB_PDO_DATA_ROLE_SWAP
//  | USB_PDO_VOLTAGE(x) | USB_PDO_CURRENT(x)
// for Orion, define USB_PDO_TYPE_FIXED
//  | USB_PDO_DUAL_ROLE | USB_PDO_HIGH_VOLTAGE
//  | USB_PDO_VOLTAGE(x) | USB_PDO_CURRENT(x)
#define USB_PDO_HIGH_VOLTAGE_SHIFT        28
#define USB_PDO_HIGH_VOLTAGE              (1 << USB_PDO_USB_SUSPEND_SHIFT)

// Variable Sink PDO (non-battery)
// define USB_PDO_TYPE_VARIABLE | USB_PDO_MAX_VOLTAGE(x) | USB_PDO_MIN_VOLTAGE(x) | USB_PDO_CURRENT(x)

// Battery Sink PDO
// define USB_PDO_TYPE_BATTERY | USB_PDO_MAX_VOLTAGE(x) | USB_PDO_MIN_VOLTAGE(x) | USB_PDO_POWER(x)

// Special values
#define PDO_VSAFE5V_SRC       (USB_PDO_TYPE_FIXED | USB_PDO_VOLTAGE(5000))
#define PDO_VSAFE5V_DFP       (USB_PDO_TYPE_FIXED | USB_PDO_DUAL_ROLE | USB_PDO_VOLTAGE(5000))
#define PDO_VSAFE0V           (USB_PDO_TYPE_FIXED | USB_PDO_VOLTAGE(0))


// Request Message
// Fixed / Variable Power Source
// define USB_REQ_OBJECT_POSITION(x) | USB_REQ_GIVE_BACK | USB_REQ_CAP_MISMATCH
//   | USB_REQ_USB_CAPABLE | USB_REQ_USB_NOSUSPEND
//   | USB_REQ_OP_CURRENT(x) | USB_REQ_MINMAX_CURRENT(x)
// for Orion, define define USB_REQ_OBJECT_POSITION(x) | USB_REQ_GIVE_BACK | USB_REQ_CAP_MISMATCH
//   | USB_REQ_USB_NOSUSPEND
//   | USB_REQ_OP_CURRENT(x) | USB_REQ_MINMAX_CURRENT(x)
#define USB_REQ_OBJECT_POSITION_SHIFT     28
#define USB_REQ_OBJECT_POSITION_MASK      0x07
#define USB_REQ_OBJECT_POSITION(x)        (((x) & USB_REQ_OBJECT_POSITION_MASK) << USB_REQ_OBJECT_POSITION_SHIFT)

#define USB_REQ_GIVE_BACK_SHIFT           27
#define USB_REQ_GIVE_BACK                 (1 << USB_REQ_GIVE_BACK_SHIFT)

#define USB_REQ_CAP_MISMATCH_SHIFT        26
#define USB_REQ_CAP_MISMATCH              (1 << USB_REQ_CAP_MISMATCH_SHIFT)

#define USB_REQ_USB_CAPABLE_SHIFT         25
#define USB_REQ_USB_CAPABLE               (1 << USB_REQ_USB_CAPABLE_SHIFT)

#define USB_REQ_USB_NOSUSPEND_SHIFT       24
#define USB_REQ_USB_NOSUSPEND             (1 << USB_REQ_USB_NOSUSPEND_SHIFT)

#define USB_REQ_RESERVED_SHIFT            20
#define USB_REQ_OP_CURRENT_SHIFT          10
#define USB_REQ_OP_CURRENT_MASK           ((1 << (USB_REQ_RESERVED_SHIFT - USB_REQ_OP_CURRENT_SHIFT)) - 1)
#define USB_REQ_OP_CURRENT(x)             ((((x) / 10) & USB_REQ_OP_CURRENT_MASK) << USB_REQ_OP_CURRENT_SHIFT)
                                            // x in mA

// field interpreted as max when Giveback = 0, or min when giveback=1
#define USB_REQ_MINMAX_CURRENT_SHIFT      0
#define USB_REQ_MINMAX_CURRENT_MASK       ((1 << (USB_REQ_OP_CURRENT_SHIFT - USB_REQ_MINMAX_CURRENT_SHIFT)) - 1)
#define USB_REQ_MINMAX_CURRENT(x)         ((((x) / 10) & USB_REQ_MINMAX_CURRENT_MASK) << USB_REQ_MINMAX_CURRENT_SHIFT)
                                            // x in mA

// Battery Power Source
// define USB_REQ_OBJECT_POSITION(x) | USB_REQ_GIVE_BACK | USB_REQ_CAP_MISMATCH
//   | USB_REQ_USB_CAPABLE | USB_REQ_USB_NOSUSPEND
//   | USB_REQ_OP_CURRENT(x) | USB_REQ_MINMAX_CURRENT(x)
// for Orion, define define USB_REQ_OBJECT_POSITION(x) | USB_REQ_GIVE_BACK | USB_REQ_CAP_MISMATCH
//   | USB_REQ_USB_NOSUSPEND
//   | USB_REQ_OP_CURRENT(x) | USB_REQ_MINMAX_CURRENT(x)
#define USB_REQ_OP_POWER_SHIFT            10
#define USB_REQ_OP_POWER_MASK             ((1 << (USB_REQ_RESERVED_SHIFT - USB_REQ_OP_POWER_SHIFT)) - 1)
#define USB_REQ_OP_POWER(x)               ((((x) / 250) & USB_REQ_OP_POWER_MASK) << USB_REQ_OP_POWER_SHIFT)
                                            // x in mW

// field interpreted as max when Giveback = 0, or min when giveback=1
#define USB_REQ_MINMAX_POWER_SHIFT        0
#define USB_REQ_MINMAX_POWER_MASK         (1 << (USB_REQ_OP_CURRENT_SHIFT - USB_REQ_MINMAX_POWER_SHIFT)) - 1
#define USB_REQ_MINMAX_POWER(x)           ((((x) / 250) & USB_REQ_MINMAX_POWER_MASK) << USB_REQ_MINMAX_POWER_SHIFT)
                                            // x in mW


#endif /* CORE_USB_PD_H_ */
