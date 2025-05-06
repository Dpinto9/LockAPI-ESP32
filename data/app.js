let video = document.getElementById('video');
let canvasElement = document.createElement('canvas');
let canvas = canvasElement.getContext('2d');
let outputData = document.getElementById('output');

let scanning = false;
let videoStream = null;

// Start scanning automatically when the page loads
document.addEventListener('DOMContentLoaded', startScanning);

async function startScanning() {
    try {
        videoStream = await navigator.mediaDevices.getUserMedia({ video: { facingMode: "environment" } });
        video.srcObject = videoStream;
        video.play();
        scanning = true;
        requestAnimationFrame(tick);
    } catch (err) {
        console.error('Error accessing camera:', err);
        outputData.textContent = `Error accessing camera: ${err.message}`;
        outputData.classList.add('error');
    }
}

function tick() {
    if (video.readyState === video.HAVE_ENOUGH_DATA && scanning) {
        canvasElement.height = video.videoHeight;
        canvasElement.width = video.videoWidth;
        canvas.drawImage(video, 0, 0, canvasElement.width, canvasElement.height);
        let imageData = canvas.getImageData(0, 0, canvasElement.width, canvasElement.height);
        
        try {
            let code = jsQR(imageData.data, imageData.width, imageData.height);
            if (code) {
                outputData.textContent = `Found QR code: ${code.data}`;
            }
        } catch (e) {
            console.error('QR detection error:', e);
        }
        
        requestAnimationFrame(tick);
    } else if (scanning) {
        requestAnimationFrame(tick);
    }
}