/**
 * ============================================================================
 * Aura v1 - Precision 3D CAD Parametric Assembly (OpenSCAD)
 * ============================================================================
 * Project: Aura Autonomous Mobile Companion Robot & Home Sentinel
 * Platform: Hack Club Fallen Hardware Grant Architecture
 * Authors: Shravan & Antigravity
 *
 * Fully Furnished Assembly & Digital Twin:
 *   1. Dual-Deck Acrylic Chassis Plates (140mm Width x 180mm Length x 3mm Thick)
 *   2. 4WD BO Geared Motors + Correctly Aligned Robotic Wheels (66mm dia)
 *      - Wheels oriented with axles along X-axis, rolling along Y-axis!
 *   3. Front Bumper with HC-SR04 Ultrasonic Sensor & Dual Auto-Docking Contact Plates
 *   4. 2-Axis MG90S Pan-Tilt Gimbal with 160° CSI Camera, Dual Floodlights & IR Diodes
 *   5. Raspberry Pi 3 Model B (Vision/Voice Brain) with USB/LAN stack & SoC
 *   6. ESP32-S3 DevKit-C N16R8 (Real-Time Kinematics & Motor Controller)
 *   7. 3S2P 18650 High-Torque Battery Pack (11.1V 4400mAh) + BMS Board
 *   8. L298N Dual H-Bridge Motor Driver with finned aluminum heatsink
 *   9. Rear 3W 4Ω Enclosed Cavity Speaker (Angled 45° UP & BACKWARD)
 *  10. INMP441 I2S MEMS Microphone Tower (Upward-facing, foam-isolated)
 *  11. DHT11 / DHT22 Ambient Temperature & Humidity Sensor
 *  12. WS2812 8-Bit RGB Status LED Ring
 * ============================================================================
 */

$fn = 40; // High smoothness for circles and cylinders

// ============================================================================
// CONFIGURATION FLAGS & DIMENSIONS
// ============================================================================
show_chassis_plates = true;
show_motors_wheels  = true;
show_front_bumper   = true;
show_gimbal_head    = true;
show_electronics    = true;
show_standoffs      = true;
exploded_offset     = 0;     // Set to 20-50 for exploded CAD inspection

// Chassis Geometric Dimensions (mm)
plate_w     = 140; // Width along X (-70 to +70)
plate_l     = 180; // Length along Y (-90 to +90) -- Front is -Y, Rear is +Y
plate_thick = 3;   // Thickness of acrylic
deck_gap    = 45;  // Clearance between lower and upper deck

standoff_x  = 58;  // Standoff distance from center (X)
standoff_y  = 78;  // Standoff distance from center (Y)

// ============================================================================
// COLOR PALETTE (Realistic Materials)
// ============================================================================
col_acrylic       = [0.12, 0.15, 0.20, 0.88]; // Smoked dark acrylic
col_brass         = [0.85, 0.70, 0.22, 1.0];  // Brass standoffs & pins
col_pcb_green     = [0.06, 0.45, 0.22, 1.0];  // Raspberry Pi green PCB
col_pcb_blue      = [0.10, 0.40, 0.82, 1.0];  // ESP32 & Driver blue PCB
col_pcb_purple    = [0.45, 0.20, 0.70, 1.0];  // INMP441 mic purple
col_motor_yellow  = [0.96, 0.68, 0.12, 1.0];  // BO Motor yellow gearbox
col_wheel_rim     = [0.98, 0.78, 0.18, 1.0];  // Yellow plastic rim
col_tire_rubber   = [0.12, 0.12, 0.14, 1.0];  // Black rubber tread
col_metal_silver  = [0.82, 0.84, 0.88, 1.0];  // Aluminum / steel shields
col_gold_dock     = [0.92, 0.75, 0.15, 1.0];  // Gold charging plates
col_battery_cell  = [0.90, 0.42, 0.12, 1.0];  // 18650 orange Li-ion wrap
col_dht_cyan      = [0.20, 0.72, 0.90, 1.0];  // DHT cyan plastic casing
col_speaker_black = [0.10, 0.10, 0.12, 1.0];  // Speaker acoustic chamber
col_led_glow      = [0.15, 0.95, 0.55, 1.0];  // WS2812 green glow

// ============================================================================
// UTILITY MODULES
// ============================================================================
module rounded_rect(w, l, h, r) {
    hull() {
        translate([-w/2 + r, -l/2 + r, 0]) cylinder(r=r, h=h);
        translate([ w/2 - r, -l/2 + r, 0]) cylinder(r=r, h=h);
        translate([-w/2 + r,  l/2 - r, 0]) cylinder(r=r, h=h);
        translate([ w/2 - r,  l/2 - r, 0]) cylinder(r=r, h=h);
    }
}

// ============================================================================
// 1. CHASSIS PLATES
// ============================================================================
module bottom_plate() {
    difference() {
        // Base perimeter with smooth 8mm fillet corners
        rounded_rect(plate_w, plate_l, plate_thick, 8);

        // 4x M3 Brass Standoff Mounting Holes (3.2mm)
        translate([-standoff_x, -standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);
        translate([ standoff_x, -standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);
        translate([-standoff_x,  standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);
        translate([ standoff_x,  standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);

        // 4x BO Motor Clamping Slots (FL, FR, RL, RR)
        for (sx = [-1, 1]) {
            for (sy = [-1, 1]) {
                translate([sx * 48 - 8, sy * 50 - 1.75, -1]) cube([16, 3.5, plate_thick+2]);
                translate([sx * 48 - 8, sy * 50 + 12.0, -1]) cube([16, 3.5, plate_thick+2]);
            }
        }

        // Battery Pack Velcro Tie-Down Slots
        translate([-30, -12, -1]) cube([60, 4, plate_thick+2]);
        translate([-30,  12, -1]) cube([60, 4, plate_thick+2]);

        // Main Wire Pass-Through Central Grommet
        translate([0, 0, -1]) cylinder(d=18, h=plate_thick+2);
    }
}

module top_plate() {
    difference() {
        // Upper deck perimeter
        rounded_rect(plate_w, plate_l, plate_thick, 8);

        // 4x M3 Standoff Holes
        translate([-standoff_x, -standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);
        translate([ standoff_x, -standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);
        translate([-standoff_x,  standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);
        translate([ standoff_x,  standoff_y, -1]) cylinder(d=3.2, h=plate_thick+2);

        // Front Pan Servo Cutout (MG90S: 23mm x 12.5mm)
        translate([-11.5, -60 - 6.25, -1]) cube([23, 12.5, plate_thick+2]);

        // Raspberry Pi 3B M2.5 Mounting Holes (58mm x 49mm, centered at X = -34, Y = -6)
        translate([-34 - 24.5, -6 - 29, -1]) cylinder(d=2.7, h=plate_thick+2);
        translate([-34 + 24.5, -6 - 29, -1]) cylinder(d=2.7, h=plate_thick+2);
        translate([-34 - 24.5, -6 + 29, -1]) cylinder(d=2.7, h=plate_thick+2);
        translate([-34 + 24.5, -6 + 29, -1]) cylinder(d=2.7, h=plate_thick+2);

        // ESP32-S3 Mount Holes (centered at X = 38, Y = -6)
        translate([38 - 11, -6 - 24, -1]) cylinder(d=2.5, h=plate_thick+2);
        translate([38 + 11, -6 - 24, -1]) cylinder(d=2.5, h=plate_thick+2);
        translate([38 - 11, -6 + 24, -1]) cylinder(d=2.5, h=plate_thick+2);
        translate([38 + 11, -6 + 24, -1]) cylinder(d=2.5, h=plate_thick+2);

        // INMP441 Mic Acoustic Tower Port (Center Island: X = 0, Y = -36)
        translate([0, -36, -1]) cylinder(d=9, h=plate_thick+2);

        // Rear 45° Speaker Sound Relief Grille (centered at X = 0, Y = 70)
        for (gx = [-14 : 7 : 14]) {
            translate([gx, 68, -1]) cylinder(d=4.2, h=plate_thick+2);
            translate([gx, 74, -1]) cylinder(d=4.2, h=plate_thick+2);
        }

        // Secondary Wire Pass-Through Port
        translate([0, 18, -1]) cylinder(d=16, h=plate_thick+2);
    }
}

// ============================================================================
// 2. 4WD BO MOTORS & ROBOTIC WHEELS (CORRECT AXLE ORIENTATION)
// ============================================================================
// Axles MUST point along X (Left/Right) and wheels MUST roll along Y (Forward/Back)!
module bo_motor_and_wheel(is_left=true) {
    // 1. BO Motor Gearbox Body (Yellow)
    color(col_motor_yellow) {
        translate([is_left ? 0 : -22, -22.5, -10])
            cube([22, 45, 20]);
    }

    // 2. Drive Axle Shaft (along X axis!)
    color(col_metal_silver) {
        translate([is_left ? -12 : 22, 0, 0])
            rotate([0, 90, 0])
                cylinder(d=5.4, h=12);
    }

    // 3. Wheel Rim & Rubber Tire (Axle along X, Rolls along Y!)
    // Both sides use positive height h=20, translated cleanly
    translate([is_left ? -32 : 32, 0, 0]) {
        // Rubber Tire (66mm outer diameter, 20mm width)
        color(col_tire_rubber) {
            rotate([0, 90, 0])
                cylinder(d=66, h=20, center=true);
        }
        // Yellow Wheel Hub / Rim
        color(col_wheel_rim) {
            rotate([0, 90, 0])
                cylinder(d=42, h=20.5, center=true);
        }
        // Center Axle Nut Cap
        color(col_metal_silver) {
            translate([is_left ? -10.5 : 10.5, 0, 0])
                rotate([0, 90, 0])
                    cylinder(d=9, h=2, center=true);
        }
    }
}

module locomotion_4wd() {
    // Front-Left Wheel & Motor
    translate([-58, -50, -10]) bo_motor_and_wheel(is_left=true);
    // Front-Right Wheel & Motor
    translate([ 58, -50, -10]) bo_motor_and_wheel(is_left=false);
    // Rear-Left Wheel & Motor
    translate([-58,  50, -10]) bo_motor_and_wheel(is_left=true);
    // Rear-Right Wheel & Motor
    translate([ 58,  50, -10]) bo_motor_and_wheel(is_left=false);
}

// ============================================================================
// 3. FRONT BUMPER, ULTRASONIC SENSOR & AUTO-DOCKING CONTACTS
// ============================================================================
module front_bumper_assembly() {
    // Sits at front edge (Y = -92), mounted low to bottom deck
    translate([0, -92, 11]) {
        // 1. Heavy-duty Bumper Bar (Black)
        color([0.15, 0.16, 0.18]) {
            difference() {
                cube([140, 10, 22], center=true);

                // Cutouts for HC-SR04 Transducer Eyes (16.2mm dia)
                translate([-13, -1, 0]) rotate([-90, 0, 0]) cylinder(d=16.2, h=12);
                translate([ 13, -1, 0]) rotate([-90, 0, 0]) cylinder(d=16.2, h=12);

                // Recesses for Left & Right Gold Charging Plates
                translate([-56, -3, 0]) cube([16.5, 6, 20.5], center=true);
                translate([ 56, -3, 0]) cube([16.5, 6, 20.5], center=true);
            }
        }

        // 2. HC-SR04 Ultrasonic Sensor Module
        translate([0, 3, 0]) {
            // Blue Sensor PCB
            color(col_pcb_blue)
                cube([45, 2, 20], center=true);

            // Left & Right Aluminum Transducer Barrels (Eyes pointing dead FORWARD!)
            color(col_metal_silver) {
                translate([-13, -8, 0]) rotate([-90, 0, 0]) cylinder(d=16, h=12);
                translate([ 13, -8, 0]) rotate([-90, 0, 0]) cylinder(d=16, h=12);
            }

            // Crystal Oscillator
            color([0.9, 0.9, 0.95])
                translate([0, -3, 0]) cube([4, 2, 10], center=true);
        }

        // 3. Left (+) & Right (-) Gold Charging Contact Plates
        color(col_gold_dock) {
            translate([-56, -5.5, 0]) cube([16, 2, 20], center=true);
            translate([ 56, -5.5, 0]) cube([16, 2, 20], center=true);
        }
    }
}

// ============================================================================
// 4. 2-AXIS PAN-TILT GIMBAL HEAD (TOP-FRONT CENTER: Y = -60)
// ============================================================================
module pan_tilt_gimbal_head() {
    translate([0, -60, deck_gap + plate_thick + exploded_offset]) {
        // 1. Pan Base Servo (MG90S Metal Gear) firmly seated in deck cutout
        color(col_pcb_blue)
            translate([0, 0, 6]) cube([23, 12.5, 24], center=true);

        // Brass Output Spline & Pan Disc
        color(col_brass)
            translate([0, 0, 19]) cylinder(d=12, h=3, center=true);

        // 2. Acrylic / Aluminum Tilt U-Bracket
        color(col_acrylic) {
            translate([0, 0, 21.5]) cube([34, 18, 3], center=true);
            translate([-16, 0, 32]) cube([3, 18, 20], center=true);
            translate([ 16, 0, 32]) cube([3, 18, 20], center=true);
        }

        // 3. Tilt Servo (Horizontally mounted in U-bracket)
        color(col_pcb_blue)
            translate([8, 0, 32]) cube([12.5, 23, 16], center=true);

        // 4. Tilt Head Sensor Platform (holding Camera, Floodlights, IR)
        translate([0, 0, 36]) {
            // Camera Module Green PCB (OV5647 160° CSI)
            color(col_pcb_green)
                cube([25, 2, 25], center=true);

            // 160° Wide-Angle Fisheye Lens Barrel (Facing FORWARD along -Y!)
            color([0.08, 0.08, 0.10])
                translate([0, -8, 0]) rotate([-90, 0, 0]) cylinder(d1=9, d2=14, h=10);

            // Dual Night Vision LED Floodlights (Flanking camera)
            color([0.98, 0.98, 0.95]) {
                translate([-18, -4, 0]) cube([7, 4, 7], center=true);
                translate([ 18, -4, 0]) cube([7, 4, 7], center=true);
            }

            // Dual Auto-Docking IR Diodes (Under camera)
            color([0.15, 0.15, 0.20]) {
                translate([-9, -4, -9]) rotate([-90, 0, 0]) cylinder(d=5, h=6);
                translate([ 9, -4, -9]) rotate([-90, 0, 0]) cylinder(d=5, h=6);
            }
        }
    }
}

// ============================================================================
// 5. ONBOARD ELECTRONICS (Pi 3B, ESP32, Battery, Audio, Sensors)
// ============================================================================
module onboard_electronics() {
    // --- A. BOTTOM DECK MUSCLE & POWER ---
    // 1. 3S2P 18650 Battery Pack (6x cells laid flat in center for ultra-low CG)
    translate([0, 0, plate_thick]) {
        color(col_battery_cell) {
            for (r = [0, 1]) {
                for (c = [-1, 0, 1]) {
                    translate([c * 19, r * 19 - 9.5, 9])
                        rotate([0, 90, 0])
                            cylinder(d=18, h=65, center=true);
                }
            }
        }
        // 3S 20A BMS Protection Board
        color(col_pcb_green)
            translate([0, -22, 10]) cube([60, 2, 18], center=true);
    }

    // 2. L298N Dual Motor Driver with Heatsink (Rear Bottom Deck)
    translate([0, 48, plate_thick]) {
        color(col_pcb_blue)
            translate([0, 0, 2]) cube([44, 44, 4], center=true);
        // Black Aluminum Heatsink
        color([0.1, 0.1, 0.12])
            translate([0, 0, 14]) cube([24, 18, 20], center=true);
    }

    // 3. DC-DC Buck Converters (XL4015 5A for Pi, LM2596 3A for Servos)
    color(col_pcb_blue) {
        translate([-46, 0, plate_thick + 5]) cube([22, 42, 10], center=true);
        translate([ 46, 0, plate_thick + 5]) cube([22, 42, 10], center=true);
    }

    // --- B. TOP DECK BRAIN & SENSORS ---
    top_z = deck_gap + plate_thick + exploded_offset;

    // 4. Raspberry Pi 3 Model B (Left Side Deck: X = -34, Y = -6)
    translate([-34, -6, top_z]) {
        // Green PCB
        color(col_pcb_green)
            translate([0, 0, 1]) cube([56, 85, 2], center=true);

        // Quad USB Stack & Ethernet Jack (Facing Rear)
        color(col_metal_silver) {
            translate([-10, 36, 9]) cube([28, 16, 16], center=true);
            translate([ 18, 36, 9]) cube([16, 15, 18], center=true);
        }

        // Broadcom BCM2837 SoC with silver heat spreader
        color([0.7, 0.72, 0.75])
            translate([0, -6, 2.5]) cube([14, 14, 2], center=true);

        // 40-Pin GPIO Header (Along outer edge)
        color([0.1, 0.1, 0.1])
            translate([-24, -8, 5]) cube([6, 52, 7], center=true);
    }

    // 5. ESP32-S3 DevKit-C (Right Side Deck: X = 38, Y = -6)
    translate([38, -6, top_z]) {
        // Blue PCB
        color(col_pcb_blue)
            translate([0, 0, 1]) cube([26, 55, 2], center=true);

        // Metal RF Shield
        color(col_metal_silver)
            translate([0, -5, 3]) cube([18, 24, 3], center=true);

        // Dual Type-C USB Ports
        color(col_metal_silver) {
            translate([0, 27, 2.5]) cube([8, 6, 3], center=true);
        }
    }

    // 6. INMP441 MEMS Microphone Tower (Acoustic Center Island: X = 0, Y = -36)
    translate([0, -36, top_z]) {
        // Acoustic damping isolation tower
        color([0.15, 0.15, 0.18])
            translate([0, 0, 6]) cylinder(d=10, h=12, center=true);

        // Purple round microphone PCB
        color(col_pcb_purple)
            translate([0, 0, 13]) cylinder(d=13, h=2, center=true);

        // Gold center acoustic diaphragm port
        color(col_gold_dock)
            translate([0, 0, 14.2]) cylinder(d=2.4, h=0.6, center=true);
    }

    // 7. 3W 4Ω Cavity Speaker (CENTER REAR: X = 0, Y = 70, ANGLED 45° UP & REARWARD)
    translate([0, 70, top_z + 8]) {
        rotate([45, 0, 0]) {
            // Black Acoustic Resonance Chamber Box (Width 50mm x Thickness 20mm x Height 36mm)
            color(col_speaker_black)
                cube([50, 20, 36], center=true);

            // Silver Speaker Driver Cone (Concentric & Symmetrical on tilted rear face)
            color(col_metal_silver)
                translate([0, 10.5, 0])
                    rotate([-90, 0, 0])
                        cylinder(d1=16, d2=28, h=5);

            // Center Dust Cap Dome
            color([0.15, 0.15, 0.18])
                translate([0, 15.6, 0])
                    rotate([-90, 0, 0])
                        cylinder(d=8, h=2);

            // Chrome Surround Bezel Ring
            color(col_brass)
                translate([0, 15.8, 0])
                    rotate([-90, 0, 0])
                        difference() {
                            cylinder(d=30, h=1);
                            translate([0, 0, -0.5]) cylinder(d=27, h=2);
                        }

            // 8. WS2812 8-Bit Straight RGB LED Strip (MOUNTED ON SPEAKER BOX TOP LIP)
            translate([0, 9, 18.5]) {
                // Black fiberglass stick PCB (50mm length x 2mm thick x 6mm wide)
                color([0.1, 0.1, 0.12])
                    cube([50, 2, 6], center=true);

                // 8x Individual 5050 Addressable RGB LEDs
                for (lx = [-21 : 6 : 21]) {
                    // White chip casing
                    color([0.95, 0.95, 0.95])
                        translate([lx, 1.2, 0])
                            cube([4.5, 1, 4.5], center=true);

                    // Bright glowing LED core
                    color(col_led_glow)
                        translate([lx, 1.8, 0])
                            cube([2.8, 1, 2.8], center=true);
                }
            }
        }
    }

    // 9. DHT11 / DHT22 Climate Sensor (Right-Rear Deck: X = 42, Y = 40)
    translate([42, 40, top_z]) {
        color(col_dht_cyan)
            translate([0, 0, 6]) cube([15, 22, 10], center=true);
    }
}

// ============================================================================
// COMPLETE 3D VEHICLE ASSEMBLY
// ============================================================================
module aura_v1_complete_assembly() {
    // 1. Bottom Chassis Plate
    if (show_chassis_plates) {
        color(col_acrylic)
            bottom_plate();
    }

    // 2. M3 Brass Standoffs (45mm length connecting decks)
    if (show_standoffs) {
        color(col_brass) {
            translate([-standoff_x, -standoff_y, plate_thick]) cylinder(d=4.5, h=deck_gap);
            translate([ standoff_x, -standoff_y, plate_thick]) cylinder(d=4.5, h=deck_gap);
            translate([-standoff_x,  standoff_y, plate_thick]) cylinder(d=4.5, h=deck_gap);
            translate([ standoff_x,  standoff_y, plate_thick]) cylinder(d=4.5, h=deck_gap);
        }
    }

    // 3. Top Chassis Plate (lifted by exploded_offset if active)
    if (show_chassis_plates) {
        translate([0, 0, deck_gap + plate_thick + exploded_offset])
            color(col_acrylic)
                top_plate();
    }

    // 4. Locomotion & 4WD Wheels (Correct Axles along X!)
    if (show_motors_wheels) {
        locomotion_4wd();
    }

    // 5. Front Bumper with Ultrasonic & Charging Plates
    if (show_front_bumper) {
        front_bumper_assembly();
    }

    // 6. 2-Axis Gimbal Head, Camera & Lights
    if (show_gimbal_head) {
        pan_tilt_gimbal_head();
    }

    // 7. Full Onboard Electronics Suite
    if (show_electronics) {
        onboard_electronics();
    }
}

// Render the complete digital twin assembly!
aura_v1_complete_assembly();
