import serial
import struct
import hashlib
from Crypto.Cipher import AES
import sys
import time

# ------------------------------
# CONFIGURATION
# ------------------------------
UART_PORT = 'COM5'       # Change to your UART port
BAUD_RATE = 115200
CHUNK_SIZE = 256
MAX_RETRIES = 3

AES_KEY = bytes.fromhex('00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff')
AES_IV  = bytes.fromhex('aabbccddeeff001122334455')

ACK_BYTE = b'\x79'  # STM32 sends ACK for successful chunk
NACK_BYTE = b'\x1F' # STM32 sends NACK for failed chunk

# ------------------------------
# MANIFEST CREATION
# ------------------------------
def create_manifest(firmware_path, slot_address, version=1):
    with open(firmware_path, 'rb') as f:
        firmware = f.read()
    size = len(firmware)
    signature = bytes([0]*64)  # Placeholder for ECDSA signature
    manifest = struct.pack('<III64s', version, size, slot_address, signature)
    return manifest, firmware

# ------------------------------
# AES-256 GCM ENCRYPTION
# ------------------------------
def encrypt_firmware(firmware):
    cipher = AES.new(AES_KEY, AES.MODE_GCM, nonce=AES_IV)
    ciphertext, tag = cipher.encrypt_and_digest(firmware)
    return ciphertext

# ------------------------------
# OTA SENDER
# ------------------------------
def send_chunk(ser, chunk):
    for attempt in range(MAX_RETRIES):
        ser.write(chunk)
        ack = ser.read(1)
        if ack == ACK_BYTE:
            return True
        else:
            print(f"[OTA] Chunk failed, retry {attempt+1}/{MAX_RETRIES}")
    return False

def send_ota(firmware_path, slot_address):
    manifest, firmware = create_manifest(firmware_path, slot_address)
    encrypted_firmware = encrypt_firmware(firmware)
    
    # Compute SHA256 of encrypted firmware for verification
    fw_hash = hashlib.sha256(encrypted_firmware).digest()
    
    ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)  # Wait for bootloader reset

    print(f"[OTA] Sending manifest ({len(manifest)} bytes)...")
    if not send_chunk(ser, manifest):
        print("[OTA] Manifest failed to send!")
        return
    
    total = len(encrypted_firmware)
    sent = 0
    print(f"[OTA] Sending firmware ({total} bytes)...")

    while sent < total:
        chunk = encrypted_firmware[sent:sent+CHUNK_SIZE]
        if not send_chunk(ser, chunk):
            print(f"[OTA] Failed at offset {sent}, aborting.")
            ser.close()
            return
        sent += len(chunk)
        print(f"\r[OTA] Sent {sent}/{total} bytes", end='')

    print("\n[OTA] Firmware transfer complete!")

    # Send hash for STM32 to verify
    print("[OTA] Sending firmware hash for verification...")
    ser.write(fw_hash)
    ack = ser.read(1)
    if ack == ACK_BYTE:
        print("[OTA] Firmware verified successfully by device!")
    else:
        print("[OTA] Firmware verification failed on device!")

    ser.close()

# ------------------------------
# MAIN
# ------------------------------
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python ota_sender_pro.py <firmware.bin> <slot_address_hex>")
        sys.exit(1)
    
    firmware_file = sys.argv[1]
    slot_addr = int(sys.argv[2], 16)
    send_ota(firmware_file, slot_addr)
