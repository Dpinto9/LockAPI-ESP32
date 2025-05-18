#include "index.h"

const char WEBPAGE_HTML[] = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>QR Code Scanner</title>
      <style>
          body {
              font-family: Arial, sans-serif;
              display: flex;
              flex-direction: column;
              align-items: center;
              background-color: #f0f0f0;
              margin: 0;
              padding: 20px;
          }
          .container {
              max-width: 800px;
              width: 100%;
              text-align: center;
          }
          #video {
              width: 100%;
              max-width: 640px;
              margin: 20px 0;
              border-radius: 8px;
              box-shadow: 0 0 10px rgba(0,0,0,0.1);
          }
          #output {
              margin: 20px 0;
              padding: 15px;
              border-radius: 8px;
              background-color: white;
              box-shadow: 0 0 10px rgba(0,0,0,0.1);
          }
          .error {
              color: red;
              margin: 10px 0;
          }
      </style>
  </head>
  <body>
      <div class="container">
          <h1>QR Code Scanner</h1>
          <video id="video" playsinline></video>
          <div id="output">No QR code detected</div>
      </div>
      <script src="https://cdn.jsdelivr.net/npm/jsqr@1.4.0/dist/jsQR.js"></script>
      <script>
      let video = document.getElementById('video');
      let canvasElement = document.createElement('canvas');
      let canvas = canvasElement.getContext('2d');
      let outputData = document.getElementById('output');
  
      let scanning = false;
      let videoStream = null;
  
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
                      fetch('/qr', {
                          method: 'POST',
                          headers: {
                              'Content-Type': 'application/json',
                          },
                          body: JSON.stringify({ qrcode: code.data })
                      });
                  }
              } catch (e) {
                  console.error('QR detection error:', e);
              }
              
              requestAnimationFrame(tick);
          } else if (scanning) {
              requestAnimationFrame(tick);
          }
      }
      </script>
  </body>
  </html>
  )rawliteral";
