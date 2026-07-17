#pragma once

#include <pgmspace.h>

// Dashboard HTML served from flash (PROGMEM) — no SRAM cost
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>WHEEL-E Dashboard</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
        body {
            background: radial-gradient(circle at center, #23120b, #0a0402);
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            color: #f5eae4;
            overflow: hidden;
        }
        .card {
            width: 90%;
            max-width: 380px;
            background: rgba(216, 122, 65, 0.03);
            border: 1px solid rgba(216, 122, 65, 0.12);
            border-radius: 28px;
            padding: 40px 25px;
            box-shadow: 0 25px 60px rgba(0, 0, 0, 0.65), inset 0 1px 1px rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(25px);
            -webkit-backdrop-filter: blur(25px);
            text-align: center;
        }
        h1 { 
            font-size: 2rem; 
            margin-bottom: 5px; 
            letter-spacing: 3px; 
            color: #e07a5f; 
            text-shadow: 0 0 12px rgba(224, 122, 95, 0.45); 
            transition: all 0.5s ease;
        }
        .card.music-mode-active h1 {
            color: #ffb703;
            text-shadow: 0 0 18px rgba(255, 183, 3, 0.7);
            letter-spacing: 5px;
        }
        .status-bar { 
            font-size: 0.85rem; 
            color: #a8928a; 
            margin-bottom: 35px; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
            gap: 8px; 
        }
        .dot { 
            width: 8px; 
            height: 8px; 
            background-color: #7c3a2d; 
            border-radius: 50%; 
            display: inline-block; 
            transition: background 0.3s; 
        }
        .dot.connected { 
            background-color: #e07a5f; 
            box-shadow: 0 0 8px #e07a5f; 
        }
        .control-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 15px;
            touch-action: none; 
        }
        .btn {
            background: rgba(216, 122, 65, 0.05);
            border: 1px solid rgba(216, 122, 65, 0.15);
            border-radius: 20px;
            aspect-ratio: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.8rem;
            color: #f5eae4;
            cursor: pointer;
            transition: background 0.08s ease, border-color 0.08s ease, box-shadow 0.08s ease;
        }
        .btn.sliding-active {
            background: #e07a5f;
            color: #0c0604;
            border-color: #e07a5f;
            box-shadow: 0 0 25px rgba(224, 122, 95, 0.55);
            transform: scale(0.94);
        }
        .btn.music-only-pad {
            opacity: 0;
            pointer-events: none;
            visibility: hidden;
            transition: opacity 0.3s ease;
        }
        .card.music-mode-active .btn {
            border-color: rgba(255, 183, 3, 0.3);
            background: rgba(255, 183, 3, 0.03);
            font-size: 1.3rem;
            font-weight: bold;
            color: #ffeeb2;
            opacity: 1;
            pointer-events: auto;
            visibility: visible;
        }
        .card.music-mode-active .btn.sliding-active {
            background: #ffb703;
            color: #023047;
            border-color: #ffb703;
            box-shadow: 0 0 25px rgba(255, 183, 3, 0.6);
        }
    </style>
</head>
<body>
    <div class="card" id="dashboardCard">
        <h1 id="mainTitle">WHEEL-E</h1>
        <div class="status-bar"><span class="dot" id="dot"></span> <span id="lbl">DISCONNECTED</span></div>
        <div class="control-grid" id="controlGrid">
            <div class="btn music-only-pad" id="nw" data-cmd="NW"></div>
            <div class="btn" id="up" data-cmd="FORWARD">▲</div>
            <div class="btn music-only-pad" id="ne" data-cmd="NE"></div>
            <div class="btn" id="left" data-cmd="LEFT">◀</div>
            <div class="btn music-only-pad" id="center" data-cmd="CENTER"></div>
            <div class="btn" id="right" data-cmd="RIGHT">▶</div>
            <div class="btn music-only-pad" id="sw" data-cmd="SW"></div>
            <div class="btn" id="down" data-cmd="BACKWARD">▼</div>
            <div class="btn music-only-pad" id="se" data-cmd="SE"></div>
        </div>
    </div>
    <script>
        let ws;
        const dot = document.getElementById('dot');
        const lbl = document.getElementById('lbl');
        const card = document.getElementById('dashboardCard');
        const title = document.getElementById('mainTitle');
        const grid = document.getElementById('controlGrid');
        
        let isMouseDown = false;
        let lastSentCommandString = "STOP";
        let lastValidCommand = "STOP";
        let lastCommandTime = 0;
        const commandDebounce = 50;  // Minimum 50ms between command updates
        
        function connect() {
            ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            ws.onopen = () => {
                dot.classList.add('connected');
                lbl.innerText = 'CONNECTED';
            };
            ws.onmessage = (evt) => {
                if (evt.data === "UI_MUSIC") setMusicUI(true);
                else if (evt.data === "UI_DRIVE") setMusicUI(false);
            };
            ws.onclose = () => {
                dot.classList.remove('connected');
                lbl.innerText = 'DISCONNECTED';
                setMusicUI(false);
                setTimeout(connect, 2000);
            };
        }
        
        function setMusicUI(isMusic) {
            if (isMusic) {
                card.classList.add('music-mode-active');
                title.innerText = "SYNTHESIZER";
                document.getElementById('nw').innerText = "C5";
                document.getElementById('up').innerText = "D5";
                document.getElementById('ne').innerText = "E5";
                document.getElementById('left').innerText = "F5";
                document.getElementById('center').innerText = "G5";
                document.getElementById('right').innerText = "A5";
                document.getElementById('sw').innerText = "B5";
                document.getElementById('down').innerText = "C6";
                document.getElementById('se').innerText = "D6";
            } else {
                card.classList.remove('music-mode-active');
                title.innerText = "WHEEL-E";
                document.getElementById('nw').innerText = "";
                document.getElementById('up').innerText = "▲";
                document.getElementById('ne').innerText = "";
                document.getElementById('left').innerText = "◀";
                document.getElementById('center').innerText = "";
                document.getElementById('right').innerText = "▶";
                document.getElementById('sw').innerText = "";
                document.getElementById('down').innerText = "▼";
                document.getElementById('se').innerText = "";
            }
        }
        
        function send(cmd) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(cmd);
            }
        }

        function evaluateInputPads(eventPoints, isEndEvent = false) {
            const now = Date.now();
            let activeCmds = [];
            let activeElements = [];
            const isMusic = card.classList.contains('music-mode-active');

            if (!isMusic) {
                // Drive mode: strict single command, reject multi-touch
                if (eventPoints.length > 1) {
                    if (isEndEvent) {
                        activeCmds = [];
                        lastValidCommand = "STOP";
                    } else {
                        activeCmds = [lastValidCommand];
                    }
                } else if (eventPoints.length === 1) {
                    const element = document.elementFromPoint(eventPoints[0].x, eventPoints[0].y);
                    if (element && element.classList.contains('btn')) {
                        if (window.getComputedStyle(element).visibility !== 'hidden') {
                            const cmd = element.getAttribute('data-cmd');
                            activeCmds.push(cmd);
                            activeElements.push(element);
                            lastValidCommand = cmd;
                        }
                    } else if (!isEndEvent) {
                        activeCmds = [lastValidCommand];
                    }
                } else if (isEndEvent) {
                    activeCmds = [];
                    lastValidCommand = "STOP";
                } else {
                    activeCmds = [lastValidCommand];
                }
            } else {
                // Synthesizer mode: full polyphonic support
                eventPoints.forEach(pt => {
                    const element = document.elementFromPoint(pt.x, pt.y);
                    if (element && element.classList.contains('btn')) {
                        if (window.getComputedStyle(element).visibility !== 'hidden') {
                            let cmd = element.getAttribute('data-cmd');
                            if (!activeCmds.includes(cmd)) {
                                activeCmds.push(cmd);
                                activeElements.push(element);
                            }
                        }
                    }
                });
            }

            document.querySelectorAll('.btn').forEach(b => {
                if (activeElements.includes(b)) b.classList.add('sliding-active');
                else b.classList.remove('sliding-active');
            });

            let commandString = activeCmds.length > 0 ? activeCmds.sort().join(",") : "STOP";
            
            // Always send STOP immediately, otherwise apply debounce
            if (commandString !== lastSentCommandString &&
                (commandString === "STOP" || now - lastCommandTime >= commandDebounce)) {
                lastSentCommandString = commandString;
                lastCommandTime = now;
                send(commandString);
            }
        }

        function releaseInputPads() {
            isMouseDown = false;
            lastValidCommand = "STOP";
            evaluateInputPads([], true);
            document.querySelectorAll('.btn').forEach(b => b.classList.remove('sliding-active'));
        }

        window.addEventListener('mousedown', (e) => {
            isMouseDown = true;
            updateMouseList(e);
        });
        window.addEventListener('mousemove', (e) => {
            if (isMouseDown) updateMouseList(e);
        });
        window.addEventListener('mouseup', releaseInputPads);
        window.addEventListener('mouseleave', releaseInputPads);
        window.addEventListener('blur', releaseInputPads);

        function updateMouseList(e) {
            evaluateInputPads([{ x: e.clientX, y: e.clientY }], false);
        }

        grid.addEventListener('touchstart', parseTouchList, { passive: false });
        grid.addEventListener('touchmove', parseTouchList, { passive: false });
        grid.addEventListener('touchend', parseTouchList, { passive: false });
        grid.addEventListener('touchcancel', parseTouchList, { passive: false });
        function parseTouchList(e) {
            e.preventDefault();
            let points = [];
            for (let i = 0; i < e.touches.length; i++) {
                points.push({ x: e.touches[i].clientX, y: e.touches[i].clientY });
            }
            const isEndEvent = e.type === 'touchend' || e.type === 'touchcancel';
            evaluateInputPads(points, isEndEvent);
        }
        
        connect();
    </script>
</body>
</html>
)rawliteral";
