#!/bin/bash
# Fix manifest files to use proper ESP32-S3 partition layout with versioned partition files

cd "$(dirname "$0")/firmware"

for firmware_file in smart-grind-by-weight-v*.bin; do
    if [[ -f "$firmware_file" && "$firmware_file" != *"-web-ota.bin" ]]; then
        # Extract version from filename
        firmware_version=$(echo "$firmware_file" | sed 's/smart-grind-by-weight-v\(.*\)\.bin/\1/')
        manifest_file="${firmware_file%.bin}.manifest.json"
        bootloader_file="${firmware_file%.bin}-bootloader.bin"
        partitions_file="${firmware_file%.bin}-partitions.bin"
        
        echo "Updating manifest for $firmware_version..."
        echo "  Bootloader: $bootloader_file"
        echo "  Partitions: $partitions_file"
        echo "  Firmware: $firmware_file"
        
        # Create manifest file with proper ESP32-S3 partition layout
        cat > "$manifest_file" << EOF
{
  "name": "Smart Grind By Weight",
  "version": "$firmware_version",
  "home_assistant_domain": "grinder",
  "new_install_skip_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        {
          "path": "$bootloader_file",
          "offset": 0
        },
        {
          "path": "$partitions_file",
          "offset": 32768
        },
        {
          "path": "blank_8KB.bin",
          "offset": 57344
        },
        {
          "path": "$firmware_file",
          "offset": 3276800
        }
      ]
    }
  ]
}
EOF
        
        echo "Generated manifest: $manifest_file"
    fi
done

echo "All manifest files updated!"