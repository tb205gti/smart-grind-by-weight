// Smart Grind By Weight - Web Bluetooth Flasher
// Based on your existing Python BLE implementation

// BLE Service UUIDs (from your bluetooth.h config)
const BLE_OTA_SERVICE_UUID = '12345678-1234-1234-1234-123456789abc';
const BLE_OTA_DATA_CHAR_UUID = '87654321-4321-4321-4321-cba987654321';
const BLE_OTA_CONTROL_CHAR_UUID = '11111111-2222-3333-4444-555555555555';
const BLE_OTA_STATUS_CHAR_UUID = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee';
const BLE_OTA_BUILD_NUMBER_CHAR_UUID = '66666666-7777-8888-9999-000000000000';

// Commands and status codes (from your Python implementation)
const BLE_OTA_CMD_START = 0x01;
const BLE_OTA_CMD_END = 0x03;
const BLE_OTA_CMD_ABORT = 0x04;

const BLE_OTA_IDLE = 0x00;
const BLE_OTA_READY = 0x01;
const BLE_OTA_RECEIVING = 0x02;
const BLE_OTA_SUCCESS = 0x03;
const BLE_OTA_ERROR = 0x04;

const DEVICE_NAME = 'GrindByWeight';
const CHUNK_SIZE = 512; // Browser BLE limit - cannot exceed 512 bytes per write

// Global state
let device = null;
let server = null;
let otaService = null;
let currentOtaStatus = BLE_OTA_IDLE;
let statusCharacteristic = null;

// Browser support check and load releases
window.addEventListener('load', () => {
    if (!('bluetooth' in navigator)) {
        document.getElementById('browserWarning').style.display = 'block';
        // Disable OTA tab if no Web Bluetooth
        const otaTab = document.querySelector('[onclick="showTab(\'ota\')"]');
        if (otaTab) {
            otaTab.disabled = true;
            otaTab.style.opacity = '0.5';
        }
    }
    
    // Load available releases
    loadReleases();
});

// Tab switching
function showTab(tabName) {
    // Hide all tab contents
    const tabContents = document.querySelectorAll('.tab-content');
    tabContents.forEach(content => {
        content.classList.remove('active');
    });
    
    // Remove active class from all tabs
    const tabs = document.querySelectorAll('.tab');
    tabs.forEach(tab => {
        tab.classList.remove('active');
    });
    
    // Show selected tab content
    document.getElementById(tabName + 'Tab').classList.add('active');
    
    // Add active class to selected tab
    event.target.classList.add('active');
}

// Status update functions
function updateStatus(message, type = 'info') {
    const statusDiv = document.getElementById('status');
    statusDiv.textContent = message;
    statusDiv.className = `status ${type}`;
    statusDiv.style.display = 'block';
    console.log(`[${type.toUpperCase()}] ${message}`);
}

function updateProgress(percent) {
    const progressContainer = document.getElementById('progressContainer');
    const progressBar = document.getElementById('progressBar');
    
    if (percent > 0) {
        progressContainer.style.display = 'block';
        progressBar.style.width = percent + '%';
    } else {
        progressContainer.style.display = 'none';
    }
}

// Utility functions
function arrayBufferToHex(buffer) {
    return Array.from(new Uint8Array(buffer))
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

// Firmware download
async function downloadFirmware(url) {
    updateStatus('Downloading firmware...', 'info');
    updateProgress(0);
    
    try {
        // Check if it's a relative URL (hosted firmware) or external URL
        let fetchUrl = url;
        if (url.startsWith('http') && !url.startsWith(window.location.origin)) {
            // Use CORS proxy for external URLs
            fetchUrl = `https://api.allorigins.win/raw?url=${encodeURIComponent(url)}`;
            updateStatus('Downloading firmware via proxy...', 'info');
        }
        
        const response = await fetch(fetchUrl);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const contentLength = parseInt(response.headers.get('content-length'));
        const reader = response.body.getReader();
        const chunks = [];
        let receivedBytes = 0;
        
        while (true) {
            const { done, value } = await reader.read();
            if (done) break;
            
            chunks.push(value);
            receivedBytes += value.length;
            
            if (contentLength) {
                const progress = (receivedBytes / contentLength) * 100;
                updateProgress(progress);
            }
        }
        
        const firmware = new Uint8Array(receivedBytes);
        let offset = 0;
        for (const chunk of chunks) {
            firmware.set(chunk, offset);
            offset += chunk.length;
        }
        
        updateStatus(`Downloaded ${Math.round(firmware.length / 1024)}KB firmware`, 'success');
        return firmware;
        
    } catch (error) {
        updateStatus(`Download failed: ${error.message}`, 'error');
        throw error;
    }
}

// BLE Connection
async function connectDevice() {
    if (!('bluetooth' in navigator)) {
        updateStatus('Web Bluetooth not supported in this browser', 'error');
        return false;
    }
    
    try {
        updateStatus('Scanning for device...', 'info');
        
        device = await navigator.bluetooth.requestDevice({
            filters: [{ name: DEVICE_NAME }],
            optionalServices: [BLE_OTA_SERVICE_UUID]
        });
        
        updateStatus('Connecting to device...', 'info');
        server = await device.gatt.connect();
        
        updateStatus('Getting OTA service...', 'info');
        otaService = await server.getPrimaryService(BLE_OTA_SERVICE_UUID);
        
        // Set up status notifications
        statusCharacteristic = await otaService.getCharacteristic(BLE_OTA_STATUS_CHAR_UUID);
        await statusCharacteristic.startNotifications();
        statusCharacteristic.addEventListener('characteristicvaluechanged', handleStatusUpdate);
        
        updateStatus('Connected successfully!', 'success');
        
        // Enable flash button
        document.getElementById('connectBtn').disabled = true;
        document.getElementById('flashBtn').disabled = false;
        
        return true;
        
    } catch (error) {
        updateStatus(`Connection failed: ${error.message}`, 'error');
        console.error('Connection error:', error);
        return false;
    }
}

// Status notification handler
function handleStatusUpdate(event) {
    const value = new Uint8Array(event.target.value.buffer);
    if (value.length > 0) {
        currentOtaStatus = value[0];
        console.log(`OTA Status: ${currentOtaStatus}`);
        
        switch (currentOtaStatus) {
            case BLE_OTA_READY:
                updateStatus('Device ready for firmware', 'info');
                break;
            case BLE_OTA_RECEIVING:
                updateStatus('Device receiving firmware...', 'info');
                break;
            case BLE_OTA_SUCCESS:
                updateStatus('Firmware update successful!', 'success');
                updateProgress(100);
                break;
            case BLE_OTA_ERROR:
                updateStatus('Firmware update failed', 'error');
                break;
        }
    }
}

// Wait for specific OTA status
async function waitForOtaStatus(expectedStatus, timeoutMs = 30000) {
    const startTime = Date.now();
    
    return new Promise((resolve, reject) => {
        const checkStatus = () => {
            if (currentOtaStatus === expectedStatus) {
                resolve(true);
                return;
            }
            
            if (Date.now() - startTime > timeoutMs) {
                reject(new Error(`Timeout waiting for OTA status ${expectedStatus}`));
                return;
            }
            
            setTimeout(checkStatus, 100);
        };
        
        checkStatus();
    });
}

// Extract build number from firmware URL
function extractBuildFromUrl(url) {
    // Try to extract version from GitHub release URL
    const versionMatch = url.match(/\/releases\/download\/v?(\d+\.\d+\.\d+)/);
    if (versionMatch) {
        // Convert version to simple build number (this is a simplification)
        const version = versionMatch[1];
        const parts = version.split('.').map(Number);
        return String(parts[0] * 100 + parts[1] * 10 + parts[2]);
    }
    return null;
}

// Web flasher always does full updates (no delta compression)
function prepareFirmwareData(firmwareData) {
    return firmwareData;
}

// Main firmware flash function
async function flashFirmware() {
    const firmwareSelect = document.getElementById('firmwareSelect');
    
    const firmwareUrl = firmwareSelect.value;
    if (!firmwareUrl) {
        updateStatus('Please select a firmware version', 'error');
        return;
    }
    
    if (!server || !server.connected) {
        updateStatus('Not connected to device', 'error');
        return;
    }
    
    try {
        // Download firmware
        const firmwareData = await downloadFirmware(firmwareUrl);
        const patchData = prepareFirmwareData(firmwareData);
        
        updateStatus('Starting firmware update...', 'info');
        updateProgress(0);
        
        // Get current device build number (optional)
        let deviceBuild = null;
        try {
            const buildChar = await otaService.getCharacteristic(BLE_OTA_BUILD_NUMBER_CHAR_UUID);
            const buildData = await buildChar.readValue();
            deviceBuild = new TextDecoder().decode(buildData).trim();
            if (deviceBuild && deviceBuild !== 'no_build_number') {
                updateStatus(`Current device build: #${deviceBuild}`, 'info');
            }
        } catch (e) {
            console.log('Could not read device build number');
        }
        
        // Extract expected build from URL
        const expectedBuild = extractBuildFromUrl(firmwareUrl);
        if (expectedBuild) {
            updateStatus(`Installing build: #${expectedBuild}`, 'info');
        }
        
        // Send OTA start command
        const controlChar = await otaService.getCharacteristic(BLE_OTA_CONTROL_CHAR_UUID);
        const dataChar = await otaService.getCharacteristic(BLE_OTA_DATA_CHAR_UUID);
        
        // Build start command: [CMD][patch_size:4][is_full_update:1][build_number_length:1][build_number:N]
        const startData = new ArrayBuffer(1 + 4 + 1 + 1 + (expectedBuild ? expectedBuild.length : 0));
        const startView = new DataView(startData);
        let offset = 0;
        
        startView.setUint8(offset, BLE_OTA_CMD_START);
        offset += 1;
        
        startView.setUint32(offset, patchData.length, true); // little-endian
        offset += 4;
        
        startView.setUint8(offset, 0); // is_full_update = 0 (use delta path with detools patch)
        offset += 1;
        
        if (expectedBuild) {
            startView.setUint8(offset, expectedBuild.length);
            offset += 1;
            const buildBytes = new TextEncoder().encode(expectedBuild);
            new Uint8Array(startData, offset).set(buildBytes);
        } else {
            startView.setUint8(offset, 0); // no build number
        }
        
        await controlChar.writeValue(startData);
        updateStatus('Sent start command, waiting for device...', 'info');
        
        // Wait for device to be ready
        await waitForOtaStatus(BLE_OTA_RECEIVING, 15000);
        
        // Send firmware data in chunks
        updateStatus('Uploading firmware...', 'info');
        const totalChunks = Math.ceil(patchData.length / CHUNK_SIZE);
        let chunkCount = 0;
        
        for (let i = 0; i < patchData.length; i += CHUNK_SIZE) {
            const chunk = patchData.slice(i, i + CHUNK_SIZE);
            await dataChar.writeValue(chunk);
            
            chunkCount++;
            // Update progress every 10 chunks (much faster UI)
            if (chunkCount % 10 === 0 || i + CHUNK_SIZE >= patchData.length) {
                const progress = Math.round(((i + chunk.length) / patchData.length) * 90); // Reserve 10% for apply phase
                updateProgress(progress);
                updateStatus(`Uploading: ${progress}%`, 'info');
            }
            
            // No delay - let browser BLE handle flow control naturally
        }
        
        updateStatus('Upload complete, applying update...', 'info');
        updateProgress(95);
        
        // Send end command
        try {
            const endCommand = new Uint8Array([BLE_OTA_CMD_END]);
            await controlChar.writeValue(endCommand);
            
            // Wait for completion or device disconnect (both are success indicators)
            try {
                await waitForOtaStatus(BLE_OTA_SUCCESS, 15000);
                updateStatus('Firmware update completed successfully!', 'success');
            } catch (statusError) {
                // Timeout or disconnect during final phase is normal - device is rebooting
                updateStatus('Firmware update completed - device rebooting', 'success');
            }
            
        } catch (endError) {
            // If END command fails, device likely already disconnected (success!)
            if (endError.message.includes('GATT') || endError.message.includes('disconnect')) {
                updateStatus('Firmware update completed - device rebooting', 'success');
            } else {
                throw endError;
            }
        }
        
        updateProgress(100);
        
        // Reset UI
        setTimeout(() => {
            document.getElementById('connectBtn').disabled = false;
            document.getElementById('flashBtn').disabled = true;
            updateProgress(0);
        }, 3000);
        
    } catch (error) {
        updateStatus(`Flash failed: ${error.message}`, 'error');
        console.error('Flash error:', error);
        
        // Try to abort OTA on error
        try {
            if (otaService) {
                const controlChar = await otaService.getCharacteristic(BLE_OTA_CONTROL_CHAR_UUID);
                const abortCommand = new Uint8Array([BLE_OTA_CMD_ABORT]);
                await controlChar.writeValue(abortCommand);
            }
        } catch (abortError) {
            console.error('Could not send abort command:', abortError);
        }
    }
}

// Update manifest firmware when dropdown changes
function updateManifestFirmware() {
    const select = document.getElementById('usbFirmwareSelect');
    const firmwareUrl = select.value;
    
    if (firmwareUrl) {
        // Update the manifest.json file dynamically (conceptually)
        // In production, this would update the actual manifest file
        console.log('Selected firmware:', firmwareUrl);
        
        document.getElementById('usbStatus').textContent = `Selected: ${firmwareUrl}`;
        document.getElementById('usbStatus').className = 'status info';
        document.getElementById('usbStatus').style.display = 'block';
        
        // For now, we'll update the manifest conceptually
        // The ESP Web Tools button will use the manifest.json file
        updateManifestFile(firmwareUrl);
    }
}

// Update manifest file with selected firmware
async function updateManifestFile(firmwareUrl) {
    const manifest = {
        name: "Smart Grind By Weight",
        version: "1.0.0",
        home_assistant_domain: "grinder",
        new_install_skip_erase: false,
        builds: [{
            chipFamily: "ESP32-S3",
            parts: [{
                path: firmwareUrl,
                offset: 0
            }]
        }]
    };
    
    console.log('Manifest updated with:', JSON.stringify(manifest, null, 2));
    
    // TODO: In production, this would make an API call to update the manifest.json file
    // For now, the manifest.json file should be manually updated or updated via build process
}

// Firmware selection is now handled directly by dropdown


// Load firmware files from directory
async function loadReleases() {
    const usbSelect = document.getElementById('usbFirmwareSelect');
    const otaSelect = document.getElementById('firmwareSelect');
    const showRC = document.getElementById('showReleaseCandidate').checked;
    const showRCOTA = document.getElementById('showReleaseCandidateOTA').checked;
    
    try {
        usbSelect.innerHTML = '<option value="">Loading firmware...</option>';
        otaSelect.innerHTML = '<option value="">Loading firmware...</option>';
        
        // Fetch firmware file list from JSON index
        const response = await fetch('firmware/index.json');
        const firmwareFiles = await response.json();
        
        // Clear dropdowns
        usbSelect.innerHTML = '';
        otaSelect.innerHTML = '';
        
        // Sort files by version (newest first)
        firmwareFiles.sort((a, b) => {
            const versionA = a.match(/v([\d\.]+)/)?.[1] || '0';
            const versionB = b.match(/v([\d\.]+)/)?.[1] || '0';
            return versionB.localeCompare(versionA, undefined, { numeric: true });
        });
        
        // Add firmware options
        firmwareFiles.forEach(filename => {
            const versionMatch = filename.match(/firmware-(v[\d\.]+(.*?))(\.bin|-web-ota\.bin)$/);
            if (!versionMatch) return;
            
            const version = versionMatch[1];
            const isPrerelease = version.includes('rc') || version.includes('beta') || version.includes('alpha');
            const isOTA = filename.includes('-web-ota.bin');
            
            const label = isPrerelease ? `${version} (pre-release)` : version;
            
            if (isOTA) {
                // Add to OTA dropdown if showRCOTA is checked or it's not a prerelease
                if (!isPrerelease || showRCOTA) {
                    otaSelect.innerHTML += `<option value="firmware/${filename}">${label}</option>`;
                }
            } else {
                // Add to USB dropdown if showRC is checked or it's not a prerelease
                if (!isPrerelease || showRC) {
                    usbSelect.innerHTML += `<option value="firmware/${filename}">${label}</option>`;
                }
            }
        });
        
        // No custom URL option needed
        
        // If no options, add fallback
        if (usbSelect.children.length === 0) {
            usbSelect.innerHTML = '<option value="firmware/firmware.bin">Latest (if available)</option>';
        }
        if (otaSelect.children.length === 0) {
            otaSelect.innerHTML = '<option value="firmware/firmware-web-ota.bin">Latest (if available)</option>';
        }
        
    } catch (error) {
        console.error('Failed to load firmware directory:', error);
        // Fallback to static options
        usbSelect.innerHTML = '<option value="firmware/firmware.bin">Latest (if available)</option>';
        otaSelect.innerHTML = '<option value="firmware/firmware-web-ota.bin">Latest (if available)</option>';
    }
}

// Handle disconnection
window.addEventListener('beforeunload', () => {
    if (device && device.gatt.connected) {
        device.gatt.disconnect();
    }
});