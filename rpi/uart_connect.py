"""
Autonomous AI Vacuum Robot - RPi to ESP32 UART Communication Bridge
Facilitates bi-directional communication between the high-level perception system
(Raspberry Pi) and the real-time motor/sensor controller (ESP32).
"""

import serial
import time

PORT = '/dev/serial0'
BAUDRATE = 115_200

ser = serial.Serial(
    port=PORT,
    baudrate=BAUDRATE,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0.2
)

# Benchmark statistics
tx_count = 0
rx_count = 0
lost_count = 0
rtts = []

try:
    print(f"AI Vacuum Robot: Connected to ESP32 on {ser.name} at {BAUDRATE} baud")
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    while True:
        tx_count += 1
        msg = f"PING:{tx_count}\n"
        
        # 1. Transmit with timestamp
        t_send = time.perf_counter()
        ser.write(msg.encode('utf-8'))

        # 2. Wait for response with round-trip measurement
        received = False
        deadline = t_send + 1.0  # 1-second budget per cycle

        while time.perf_counter() < deadline:
            if ser.in_waiting > 0:
                raw_data = ser.readline()
                decoded = raw_data.decode('utf-8', errors='replace').strip()
                
                if decoded:
                    t_recv = time.perf_counter()
                    rtt_ms = (t_recv - t_send) * 1000.0
                    rtts.append(rtt_ms)
                    rx_count += 1
                    received = True
                    
                    packet_loss = (lost_count / tx_count) * 100
                    avg_rtt = sum(rtts) / len(rtts)
                    
                    print(
                        f"[TX #{tx_count:04d}] -> Sent | "
                        f"[RX #{rx_count:04d}] <- \"{decoded}\" | "
                        f"RTT: {rtt_ms:6.2f} ms (Avg: {avg_rtt:5.2f} ms) | "
                        f"Loss: {packet_loss:4.1f}%"
                    )
                    break
            time.sleep(0.005)

        if not received:
            lost_count += 1
            packet_loss = (lost_count / tx_count) * 100
            print(f"[TX #{tx_count:04d}] -> Sent | TIMEOUT / PACKET LOST | Loss: {packet_loss:4.1f}%")

        # Sleep remainder of the 1-second slot
        remaining = deadline - time.perf_counter()
        if remaining > 0:
            time.sleep(remaining)

except KeyboardInterrupt:
    print("\n\n--- Final Benchmark Summary ---")
    if tx_count > 0:
        loss_pct = (lost_count / tx_count) * 100
        avg_rtt = sum(rtts) / len(rtts) if rtts else 0
        min_rtt = min(rtts) if rtts else 0
        max_rtt = max(rtts) if rtts else 0
        print(f"Packets Transmitted : {tx_count}")
        print(f"Packets Received    : {rx_count}")
        print(f"Packets Dropped     : {lost_count} ({loss_pct:.2f}%)")
        print(f"Latency Min/Avg/Max : {min_rtt:.2f} ms / {avg_rtt:.2f} ms / {max_rtt:.2f} ms")

finally:
    ser.close()
    print("Serial port closed.")