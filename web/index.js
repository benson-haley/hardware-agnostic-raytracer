// Main.
window.onload = function () {
    // Obtain a handle to the canvas.
    const canvasElement = document.getElementById("frame");
    const canvas = canvasElement.getContext("2d");
    // Create a new connection to a web socket server.
    socket = new WebSocket("ws://localhost:8080");
    socket.binaryType = "arraybuffer";
    // Event listener to be called upon every connection event.
    socket.addEventListener('open', (event) => {
        console.log('Connected to web socket server.');
    });
    // Event listener to be called upon every disconnect event.
    socket.addEventListener('close', (event) => {
        console.log('Disconnected from web socket server.');
    });
    // Event listener to be called upon every web socket server error.
    socket.addEventListener('error', (event) => {
        console.error('Error from web socket server:', event);
    });
    // Event listener to be called every time a message is received from the web socket server.
    socket.addEventListener('message', (event) => {
        // Draw the frame onto the canvas.
        const frameBuffer = new Uint8ClampedArray(event.data);
        const imageData = new ImageData(frameBuffer, canvasElement.width, canvasElement.height);
        canvas.putImageData(imageData, 0, 0);
    });
    // Add keyboard controls.
    let currentActions = new Set()
    document.addEventListener('keydown', (e) => {
        if (e.code == "KeyW" || e.key == "ArrowUp") {
            currentActions.add("move_forward");
        }
        if (e.code == "KeyS" || e.key == "ArrowDown") {
            currentActions.add("move_backward");
        }
        if (e.code == "KeyA" || e.key == "ArrowLeft") {
            currentActions.add("move_left");
        }
        if (e.code == "KeyD" || e.key == "ArrowRight") {
            currentActions.add("move_right");
        }
        if (e.code == "Space") {
            currentActions.add("move_up");
        }
        if (e.code == "ShiftLeft") {
            currentActions.add("move_down");
        }
    });
    document.addEventListener('keyup', (e) => {
        if (e.code == "KeyW" || e.key == "ArrowUp") {
            currentActions.delete("move_forward");
        }
        if (e.code == "KeyS" || e.key == "ArrowDown") {
            currentActions.delete("move_backward");
        }
        if (e.code == "KeyA" || e.key == "ArrowLeft") {
            currentActions.delete("move_left");
        }
        if (e.code == "KeyD" || e.key == "ArrowRight") {
            currentActions.delete("move_right");
        }
        if (e.code == "Space") {
            currentActions.delete("move_up");
        }
        if (e.code == "ShiftLeft") {
            currentActions.delete("move_down");
        }
    });
    setInterval(() => {
        for (let action of currentActions) {
            console.log(action);
            socket.send(action);
        }
    }, 16);
}