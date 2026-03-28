function esc(s) {
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

function rssiToBars(rssi) {
    const f = '&#9608;'; // █
    const e = '&#9617;'; // ░
    if (rssi > -50) return f + f + f + f;
    if (rssi > -65) return f + f + f + e;
    if (rssi > -75) return f + f + e + e;
    return f + e + e + e;
}

function sel(ssid) {
    document.getElementById('ssid').value = ssid;
    document.getElementById('pass').focus();
}

async function loadNetworks() {
    const container = document.getElementById('networks');
    container.innerHTML = '<p class="muted">Scanning&hellip;</p>';
    try {
        const r = await fetch('/api/networks');
        const networks = await r.json();

        if (!networks.length) {
            container.innerHTML = '<p class="muted">No networks found. <a href="#" id="retry-link">Retry</a></p>';
            document.getElementById('retry-link').onclick = e => { e.preventDefault(); loadNetworks(); };
            return;
        }

        container.innerHTML = networks.map(n => {
            const onclickSsid = n.ssid.replace(/\\/g, '\\\\').replace(/'/g, "\\'");
            return `<button class="net" onclick="sel('${onclickSsid}')">
                <span>${esc(n.ssid)}</span>
                <span class="sig">${rssiToBars(n.rssi)}${n.encrypted ? ' &#128274;' : ''}</span>
            </button>`;
        }).join('');
    } catch {
        container.innerHTML = '<p class="muted">Scan failed. <a href="#" id="retry-link">Retry</a></p>';
        document.getElementById('retry-link').onclick = e => { e.preventDefault(); loadNetworks(); };
    }
}

document.getElementById('connect-form').onsubmit = async function(e) {
    e.preventDefault();
    const ssid = document.getElementById('ssid').value.trim();
    const pass = document.getElementById('pass').value;

    if (!ssid) {
        alert('Please enter or select a network.');
        return;
    }

    try {
        const r = await fetch('/connect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: new URLSearchParams({ ssid, pass }).toString()
        });
        const data = await r.json();
        if (data.ok) {
            document.getElementById('main').innerHTML =
                `<div class="message">
                    <h2>Connecting&hellip;</h2>
                    <p>Credentials saved. The clock will restart and connect to <strong>${esc(data.ssid)}</strong>.</p>
                    <p>If successful the WiFi LED turns green.<br>If not, the clock returns to AP mode.</p>
                </div>`;
        }
    } catch {
        // Device likely restarted before the response was fully received.
        document.getElementById('main').innerHTML =
            `<div class="message"><h2>Connecting&hellip;</h2><p>The clock is restarting. Please wait.</p></div>`;
    }
};

document.getElementById('reset-btn').onclick = async function() {
    if (!confirm('Remove saved credentials and restart?')) return;
    try {
        await fetch('/reset', { method: 'POST' });
    } catch { /* device restarted before response */ }
    document.getElementById('main').innerHTML =
        `<div class="message">
            <h2>Credentials reset</h2>
            <p>Saved credentials removed. The clock will restart using built-in credentials.</p>
        </div>`;
};

document.getElementById('refresh-btn').onclick = loadNetworks;

loadNetworks();
