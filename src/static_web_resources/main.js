/*
* JavaScript code for the web interface
* All functionality contained in one file to minimize number of connection contexts of the thingy
*/


//////////////////////////////////////////////////////////////////
// INFO: Map functionality
//////////////////////////////////////////////////////////////////
var initialCoordinates = [16, 14];
var initialUncertainty = 0;

var map = L.map('map').setView(initialCoordinates, 1);
var marker = L.marker(initialCoordinates);
var uncertaintyCircle = L.circle(initialCoordinates, initialUncertainty, {
    color: 'blue',
    fillColor: '#blue',
    fillOpacity: 0.5,
    radius: initialUncertainty
});

L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
}).addTo(map);


function updateMarker(lat, lon, uncertainty, zoom) {
    var newCoordinates = [lat, lon];

    marker.setLatLng(newCoordinates).addTo(map);
    uncertaintyCircle.setLatLng(newCoordinates);
    uncertaintyCircle.setRadius(uncertainty).addTo(map);
    map.setView(newCoordinates, zoom);
}


document.getElementById('color_picker').addEventListener('input', function () {
    const selectedColor = this.value;
    document.getElementById('color_preview').style.backgroundColor = selectedColor;
});


////////////////////////////////////////////////////////////////
// Chart functionality
////////////////////////////////////////////////////////////////
let accel0_chart = new Highcharts.Chart({
    chart: {
        renderTo: 'chart_accel0'
    },
    title: {
        text: 'Accelerometer'
    },
    series: [{
        name: 'X',
        color: 'red',
        data: [],
        marker: { enabled: false }
    }, {
        name: 'Y',
        color: 'green',
        data: [],
        marker: { enabled: false }
    }, {
        name: 'Z',
        color: 'blue',
        data: [],
        marker: { enabled: false }
    }],
    xAxis: {
        title: {
            text: 'Seconds'
        }
    },
    yAxis: {
        title: {
            text: 'Acceleration (m/s^2)'
        }
    }
});


// NOTE: The accelerometer ADXL367 is not plotted in the web interface as this is the same data as the BMI270
// let accel1_chart = new Highcharts.Chart({
//     chart: {
//         renderTo: 'chart_accel1'
//     },
//     title: {
//         text: 'Accelerometer ADXL367'
//     },
//     series: [{
//         name: 'X',
//         color: 'red',
//         data: []
//     }, {
//         name: 'Y',
//         color: 'green',
//         data: []
//     }, {
//         name: 'Z',
//         color: 'blue',
//         data: []
//     }],
//     xAxis: {
//         title: {
//             text: 'Seconds'
//         }
//     },
//     yAxis: {
//         title: {
//             text: 'Acceleration (m/s^2)'
//         }
//     }
// });

let gyro0_chart = new Highcharts.Chart({
    accessibility: {
        enabled: false
    },
    chart: {
        renderTo: 'chart_gyro0'
    },
    title: {
        // text: 'Gyroscope BMI270'
        text: 'Gyroscope'
    },
    series: [{
        name: 'X',
        color: 'red',
        data: [],
        marker: { enabled: false }
    }, {
        name: 'Y',
        color: 'green',
        data: [],
        marker: { enabled: false }
    }, {
        name: 'Z',
        color: 'blue',
        data: [],
        marker: { enabled: false }
    }],
    xAxis: {
        title: {
            text: 'Seconds'
        }
    },
    yAxis: {
        title: {
            text: 'Angular velocity (rad/s)'
        }
    }
});

// NOTE: bmm350 is not plotted as it has no drivers in zephyr yet
// let mag0_chart = new Highcharts.Chart({
//     chart: {
//         renderTo: 'chart_mag0'
//     },
//     title: {
//         text: 'Magnetometer'
//     },
//     series: [{
//         name: 'X',
//         color: 'red',
//         data: [],
//         marker: { enabled: false }
//     }, {
//         name: 'Y',
//         color: 'green',
//         data: [],
//         marker: { enabled: false }
//     }, {
//         name: 'Z',
//         color: 'blue',
//         data: [],
//         marker: { enabled: false }
//     }],
//     xAxis: {
//         title: {
//             text: 'Seconds'
//         }
//     },
//     yAxis: {
//         title: {
//             text: 'Magnetic field (uT)'
//         }
//     }
// });

let temp_chart = new Highcharts.Chart({
    chart: {
        renderTo: 'chart_temp'
    },
    title: {
        text: 'Temperature'
    },
    series: [{
        name: 'Temperature',
        color: 'red',
        data: [],
        marker: { enabled: false }
    }],
    xAxis: {
        title: {
            text: 'Seconds'
        }
    },
    yAxis: {
        title: {
            text: 'Temperature (°C)'
        }
    }
});

let hum_chart = new Highcharts.Chart({
    chart: {
        renderTo: 'chart_humidity'
    },
    title: {
        text: 'Humidity'
    },
    series: [{
        name: 'Humidity',
        color: 'blue',
        data: [],
        marker: { enabled: false }
    }],
    xAxis: {
        title: {
            text: 'Seconds'
        }
    },
    yAxis: {
        title: {
            text: 'Humidity (%)'
        }
    }
});

let press_chart = new Highcharts.Chart({
    chart: {
        renderTo: 'chart_pressure'
    },
    title: {
        text: 'Pressure'
    },
    series: [{
        name: 'Pressure',
        color: 'green',
        data: [],
        marker: { enabled: false }
    }],
    xAxis: {
        title: {
            text: 'Seconds'
        }
    },
    yAxis: {
        title: {
            text: 'Pressure (hPa)'
        }
    }
});

// NOTE: The gas resistance is not plotted in the web interface as this value seems to be incorrect
// let gas_chart = new Highcharts.Chart({
//     chart: {
//         renderTo: 'chart_gas'
//     },
//     title: {
//         text: 'Gas resistance'
//     },
//     series: [{
//         name: 'Gas resistance',
//         color: 'purple',
//         data: []
//     }],
//     xAxis: {
//         title: {
//             text: 'Seconds'
//         }
//     },
//     yAxis: {
//         title: {
//             text: 'Gas resistance (Ohm)'
//         }
//     }
// });

function updatePlots(data) {
    // let x = (new Date()).getTime();
    let x = parseFloat(data.timestamp);
    let y = parseFloat(data.bmi270_ax);
    if (accel0_chart.series[0].data.length > 100) {
        accel0_chart.series[0].addPoint([x, y], true, true, false);
    } else {
        accel0_chart.series[0].addPoint([x, y], true, false, false);
    }
    y = parseFloat(data.bmi270_ay);
    if (accel0_chart.series[1].data.length > 100) {
        accel0_chart.series[1].addPoint([x, y], true, true, false);
    } else {
        accel0_chart.series[1].addPoint([x, y], true, false, false);
    }
    y = parseFloat(data.bmi270_az);
    if (accel0_chart.series[2].data.length > 100) {
        accel0_chart.series[2].addPoint([x, y], true, true, false);
    } else {
        accel0_chart.series[2].addPoint([x, y], true, false, false);
    }

    //NOTE: The accelerometer ADXL367 is not plotted in the web interface as this is the same data as the BMI270
    // y = parseFloat(data.adxl_ax);
    // if (accel1_chart.series[0].data.length > 1000) {
    //     accel1_chart.series[0].addPoint([x, y], true, true, false);
    // } else {
    //     accel1_chart.series[0].addPoint([x, y], true, false, false);
    // }
    // y = parseFloat(data.adxl_ay);
    // if (accel1_chart.series[1].data.length > 1000) {
    //     accel1_chart.series[1].addPoint([x, y], true, true, false);
    // } else {
    //     accel1_chart.series[1].addPoint([x, y], true, false, false);
    // }
    // y = parseFloat(data.adxl_az);
    // if (accel1_chart.series[2].data.length > 1000) {
    //     accel1_chart.series[2].addPoint([x, y], true, true, false);
    // } else {
    //     accel1_chart.series[2].addPoint([x, y], true, false, false);
    // }

    y = parseFloat(data.bmi270_gx);
    if (gyro0_chart.series[0].data.length > 100) {
        gyro0_chart.series[0].addPoint([x, y], true, true, false);
    } else {
        gyro0_chart.series[0].addPoint([x, y], true, false, false);
    }
    y = parseFloat(data.bmi270_gy);
    if (gyro0_chart.series[1].data.length > 100) {
        gyro0_chart.series[1].addPoint([x, y], true, true, false);
    } else {
        gyro0_chart.series[1].addPoint([x, y], true, false, false);
    }
    y = parseFloat(data.bmi270_gz);
    if (gyro0_chart.series[2].data.length > 100) {
        gyro0_chart.series[2].addPoint([x, y], true, true, false);
    } else {
        gyro0_chart.series[2].addPoint([x, y], true, false, false);
    }

    //NOTE: bmm350 is not plotted as it has no drivers in zephyr yet
    // y = parseFloat(data.bmm350_magn_x);
    // if (mag0_chart.series[0].data.length > 100) {
    //     mag0_chart.series[0].addPoint([x, y], true, true, false);
    // } else {
    //     mag0_chart.series[0].addPoint([x, y], true, false, false);
    // }
    // y = parseFloat(data.bmm350_magn_y);
    // if (mag0_chart.series[1].data.length > 100) {
    //     mag0_chart.series[1].addPoint([x, y], true, true, false);
    // } else {
    //     mag0_chart.series[1].addPoint([x, y], true, false, false);
    // }
    // y = parseFloat(data.bmm350_magn_z);
    // if (mag0_chart.series[2].data.length > 100) {
    //     mag0_chart.series[2].addPoint([x, y], true, true, false);
    // } else {
    //     mag0_chart.series[2].addPoint([x, y], true, false, false);
    // }

    y = parseFloat(data.bme680_temperature);
    if (temp_chart.series[0].data.length > 1000) {
        temp_chart.series[0].addPoint([x, y], true, true, false);
    } else {
        temp_chart.series[0].addPoint([x, y], true, false, false);
    }

    y = parseFloat(data.bme680_humidity);
    if (hum_chart.series[0].data.length > 1000) {
        hum_chart.series[0].addPoint([x, y], true, true, false);
    } else {
        hum_chart.series[0].addPoint([x, y], true, false, false);
    }

    y = parseFloat(data.bme680_pressure);
    if (press_chart.series[0].data.length > 1000) {
        press_chart.series[0].addPoint([x, y], true, true, false);
    } else {
        press_chart.series[0].addPoint([x, y], true, false, false);
    }

    //NOTE: The gas resistance is not plotted in the web interface as this value seems to be incorrect
    // y = parseFloat(data.bme680_gas);
    // if (gas_chart.series[0].data.length > 1000) {
    //     gas_chart.series[0].addPoint([x, y], true, true, false);
    // } else {
    //     gas_chart.series[0].addPoint([x, y], true, false, false);
    // }
}

function setSensorData(json_data, sensor_name) {
    // document.getElementById(sensor_name).innerHTML = json_data[sensor_name];
    document.getElementById(sensor_name).innerHTML = json_data[sensor_name].toFixed(3);
}

////////////////////////////////////////////////////////////////
// 3D Model
////////////////////////////////////////////////////////////////

class MovingAverageFilter {
    constructor(windowSize) {
        this.windowSize = windowSize;
        this.values = [];
        this.sum = 0;
    }

    add(value) {
        this.values.push(value);
        this.sum += value;

        if (this.values.length > this.windowSize) {
            this.sum -= this.values.shift();
        }
    }

    getAverage() {
        let result = this.sum / this.values.length;
        if (isNaN(result)) {
            return 0;
        }
        return result;
    }
}

const windowSize = 100; // Adjust the window size as needed
const maFilterGx = new MovingAverageFilter(windowSize, 0.01);
const maFilterGy = new MovingAverageFilter(windowSize, 0.01);
const maFilterGz = new MovingAverageFilter(windowSize, 0.01);

let old_time = 0;
let orientationQuat = { w: 1, x: 0, y: 0, z: 0 };

// Convert radians to degrees
function radToDeg(radians) {
    return radians * 180 / Math.PI;
}

// Convert degrees to radians
function degToRad(degrees) {
    return degrees * Math.PI / 180;
}

// Normalize a quaternion
function normalizeQuaternion(q) {
    const length = Math.sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    return {
        w: q.w / length,
        x: q.x / length,
        y: q.y / length,
        z: q.z / length
    };
}

// Multiply two quaternions
function multiplyQuaternions(q1, q2) {
    return {
        w: q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
        x: q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        y: q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        z: q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w
    };
}

// Convert gyroscope delta (deg/s) to a quaternion
function toDeltaQuaternion(gyroX, gyroY, gyroZ, deltaTime) {

    const halfDeltaX = gyroX * deltaTime / 2;
    const halfDeltaY = gyroY * deltaTime / 2;
    const halfDeltaZ = gyroZ * deltaTime / 2;

    return normalizeQuaternion({
        w: Math.cos(halfDeltaX) * Math.cos(halfDeltaY) * Math.cos(halfDeltaZ) + Math.sin(halfDeltaX) * Math.sin(halfDeltaY) * Math.sin(halfDeltaZ),
        x: Math.sin(halfDeltaX) * Math.cos(halfDeltaY) * Math.cos(halfDeltaZ) - Math.cos(halfDeltaX) * Math.sin(halfDeltaY) * Math.sin(halfDeltaZ),
        y: Math.cos(halfDeltaX) * Math.sin(halfDeltaY) * Math.cos(halfDeltaZ) + Math.sin(halfDeltaX) * Math.cos(halfDeltaY) * Math.sin(halfDeltaZ),
        z: Math.cos(halfDeltaX) * Math.cos(halfDeltaY) * Math.sin(halfDeltaZ) - Math.sin(halfDeltaX) * Math.sin(halfDeltaY) * Math.cos(halfDeltaZ)
    });
}

// Convert a quaternion to intrinsic Roll (Z), Pitch (X), Yaw (Y) Euler angles
function quaternionToEuler(q) {
    const sinPitch = -2 * (q.x * q.z - q.w * q.y);
    const pitch = Math.asin(Math.min(Math.max(sinPitch, -1), 1)); // Clamping for numerical stability

    const yaw = Math.atan2(2 * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
    const roll = Math.atan2(2 * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);

    return {
        roll: radToDeg(roll),
        pitch: radToDeg(pitch),
        yaw: radToDeg(yaw)
    };
}

// Construct a quaternion from Euler angles
function eulerToQuaternion(roll, pitch, yaw) {
    const halfRoll = degToRad(roll) / 2;
    const halfPitch = degToRad(pitch) / 2;
    const halfYaw = degToRad(yaw) / 2;

    const sinRoll = Math.sin(halfRoll);
    const cosRoll = Math.cos(halfRoll);
    const sinPitch = Math.sin(halfPitch);
    const cosPitch = Math.cos(halfPitch);
    const sinYaw = Math.sin(halfYaw);
    const cosYaw = Math.cos(halfYaw);

    return normalizeQuaternion({
        w: cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw,
        x: sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
        y: cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
        z: cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw
    });
}

function rollPitchQuatFromAcc(accelX, accelY, accelZ) {

    // Normalize the accelerometer data
    const norm = Math.sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
    accelX /= norm;
    accelY /= norm;
    accelZ /= norm;

    accelZ = -accelZ; 

    // Pitch quaternion
    const s_theta = Math.min(Math.max(accelX, -1), 1);
    const c_theta = Math.sqrt(1 - s_theta * s_theta);
    const s_theta_half = Math.sign(s_theta) * Math.sqrt((1 - c_theta) / 2);
    const c_theta_half = Math.sqrt((1 + c_theta) / 2);

    let pitchQuat = {
        w: c_theta_half,
        x: 0,
        y: s_theta_half,
        z: 0
    };

    pitchQuat = normalizeQuaternion(pitchQuat);

    // Roll quaternion
    const is_singular = (c_theta == 0);
    const s_phi = is_singular ? 0 : (-1 * accelY / c_theta);
    let c_phi = is_singular ? 0 : (-1 * accelZ / c_theta);
    c_phi = Math.min(Math.max(c_phi, -1), 1);
    let sign_s_phi = Math.sign(s_phi);
    if(c_phi == -1.0 && s_phi == 0.0) {
        sign_s_phi = 1.0;
    }
    const s_phi_half = sign_s_phi * Math.sqrt((1 - c_phi) / 2);
    const c_phi_half = Math.sqrt((1 + c_phi) / 2);

    let rollQuat = {
        w: c_phi_half,
        x: s_phi_half,
        y: 0,
        z: 0
    };
    rollQuat = normalizeQuaternion(rollQuat);

    // Combine the pitch and roll quaternions
    const combinedQuat = multiplyQuaternions(pitchQuat, rollQuat);
    return normalizeQuaternion(combinedQuat);
}

function updateOrientation(data) {
    let time = parseFloat(data.timestamp);
    if (old_time == 0) {
        old_time = time;
    }
    let delta_time = time - old_time;
    old_time = time;

    let gx = parseFloat(data.bmi270_gx);
    let gy = parseFloat(data.bmi270_gy);
    let gz = parseFloat(data.bmi270_gz);

    let ax = parseFloat(data.bmi270_ax);
    let ay = parseFloat(data.bmi270_ay);
    let az = parseFloat(data.bmi270_az);

    // Gyro offset compensation
    if((Math.abs(gx) < 0.01) && (Math.abs(gy) < 0.01) && (Math.abs(gz) < 0.01)) {
        maFilterGx.add(gx);
        maFilterGy.add(gy);
        maFilterGz.add(gz);
    }

    gx -= maFilterGx.getAverage();
    gy -= maFilterGy.getAverage();
    gz -= maFilterGz.getAverage();

    const deltaQuaternion = toDeltaQuaternion(-gx, -gy, gz, delta_time);
    orientationQuat = multiplyQuaternions(orientationQuat, deltaQuaternion);
    orientationQuat = normalizeQuaternion(orientationQuat);

    accQuat = rollPitchQuatFromAcc(ax, ay, az);

    // console.log("accQuat: " + accQuat.w + " " + accQuat.x + " " + accQuat.y + " " + accQuat.z);
    // console.log("orientationQuat: " + orientationQuat.w + " " + orientationQuat.x + " " + orientationQuat.y + " " + orientationQuat.z);

    // // Complementary filter
    // const alpha = 0.8;
    // orientationQuat.w = alpha * orientationQuat.w + (1 - alpha) * accQuat.w;
    // orientationQuat.x = alpha * orientationQuat.x + (1 - alpha) * accQuat.x;
    // orientationQuat.y = alpha * orientationQuat.y + (1 - alpha) * accQuat.y;
    // orientationQuat.z = alpha * orientationQuat.z + (1 - alpha) * accQuat.z;
    // orientationQuat = normalizeQuaternion(orientationQuat);

    let euler = quaternionToEuler(orientationQuat);
    const accEuler = quaternionToEuler(accQuat);

    // console.log("euler:" + euler.roll + " " + euler.pitch + " " + euler.yaw);
    // console.log("accEuler:" + accEuler.roll + " " + accEuler.pitch + " " + accEuler.yaw);

    // complementary filter
    const alpha = 0.8;
    euler.roll = alpha * euler.roll + (1 - alpha) * accEuler.roll;
    euler.pitch = alpha * euler.pitch + (1 - alpha) * accEuler.pitch;

    // Feedback after the complementary filter
    orientationQuat = eulerToQuaternion(euler.roll, euler.pitch, euler.yaw);


    plotOrientation(euler.roll, euler.pitch, euler.yaw);

}

function plotOrientation(roll, pitch, yaw) {

    // console.log("Roll: " + roll + " Pitch: " + pitch + " Yaw: " + yaw);

    const modelViewerTransform = document.querySelector("model-viewer#transform");
    modelViewerTransform.orientation = `${roll}deg ${pitch}deg ${yaw}deg`;
}

// Wait for the site to load before loading the 3D model
window.addEventListener('load', function () {
    var modelViewer = document.getElementById('transform');
    modelViewer.setAttribute('src', modelViewer.getAttribute('data-src'));
});

////////////////////////////////////////////////////////////////////////////
// Function to post RGB LED data
////////////////////////////////////////////////////////////////////////////
async function postRgbLed(hex_color) {

    let r = parseInt(hex_color[0]);
    let g = parseInt(hex_color[1]);
    let b = parseInt(hex_color[2]);

    try {
        const payload = JSON.stringify({ "r": r, "g": g, "b": b });

        // console.log(payload);

        const response = await fetch("/led", { method: "POST", body: payload });
        if (!response.ok) {
            throw new Error(`Response status: ${response.status}`);
        }
    }
    catch (error) {
        console.error(error.message);
    }
}

////////////////////////////////////////////////////////////////////////////
// Function to fetch the location
////////////////////////////////////////////////////////////////////////////

const fetchInterval = 5000; // 5 seconds

function fetchLocation(attempts_left) {
    console.log(attempts_left);
    fetch("/location")
        .then(response => {
            if (!response.ok) {
                throw new Error(`Response status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            // {lat: 16.0, lon: 14.0, uncertainty: 0.0}
            // {message: "Location not available"}

            console.log(data);

            if (data.message !== undefined) {
                console.log("Location not available");
                console.log(data.message);
                document.getElementById("jwt-error").innerHTML = data.message;

            } else if (data.lat !== undefined && data.lon !== undefined && data.uncertainty !== undefined) {
                console.log("Location available");
                updateMarker(data.lat, data.lon, data.uncertainty, 15);
            }
        })
        .catch(error => {
            console.error(error.message);
            if (attempts_left > 1) {
                setTimeout(() => {
                    fetchLocation(attempts_left - 1);
                }, fetchInterval);
            }
        });
}

async function postJWT(JWT) {

    const payload = JSON.stringify({ "jwt": JWT });
    console.log(payload);

    try {
        const response = await fetch("/jwt", { method: "POST", body: payload });
        if (!response.ok) {
            throw new Error(`Response status: ${response.status}`);
        }
    }
    catch (error) {
        console.error(error.message);
    }
}

////////////////////////////////////////////////////////////////////////////
// Setup after the page loads
////////////////////////////////////////////////////////////////////////////

// Buttons
window.addEventListener("DOMContentLoaded", (ev) => {

    // Setup the event listeners for the buttons
    document.getElementById('reset-orientation').addEventListener('click', function () {
        orientationQuat = { w: 1, x: 0, y: 0, z: 0 };
    });
    document.getElementById('jwt-submit').addEventListener('click', async function () {
        const jwt = document.getElementById('jwt-input').value;
        document.getElementById("jwt-error").innerHTML = "";

        postJWT(jwt);

        // Clear the input field
        document.getElementById('jwt-input').value = "";

        // Restart the fetch attempts
        await new Promise(r => setTimeout(r, 5000));
        fetchLocation(5);
    });

    // attempt to fetch the location after boot
    fetchLocation(1);

});

// WebSocket connection
document.addEventListener('DOMContentLoaded', (event) => {
    /* Setup websocket for handling network stats */
    const ws = new WebSocket("/");
    ws.onopen = (event) => {
        console.log("Connected to the server");
    }

    ws.onmessage = (event) => {
        // console.log("Received data");

        const data = JSON.parse(event.data);

        
        //NOTE: The accelerometer ADXL367 is not plotted in the web interface as this is the same data as the BMI270
        // setSensorData(data, "adxl_ax");
        // setSensorData(data, "adxl_ay");
        // setSensorData(data, "adxl_az");

        setSensorData(data, "bme680_temperature");
        setSensorData(data, "bme680_humidity");
        setSensorData(data, "bme680_pressure");
        //NOTE: The gas resistance is not plotted in the web interface as this value seems to be incorrect
        // setSensorData(data, "bme680_gas");

        setSensorData(data, "bmi270_gx");
        setSensorData(data, "bmi270_gy");
        setSensorData(data, "bmi270_gz");
        setSensorData(data, "bmi270_ax");
        setSensorData(data, "bmi270_ay");
        setSensorData(data, "bmi270_az");

        //NOTE: bmm350 is not plotted as it has no drivers in zephyr yet
        // setSensorData(data, "bmm350_magn_x");
        // setSensorData(data, "bmm350_magn_y");
        // setSensorData(data, "bmm350_magn_z");

        updatePlots(data);
        updateOrientation(data);
    
    }
    
    window.addEventListener('beforeunload', function () {
        ws.close();
    });
});

// Color wheel
document.addEventListener('DOMContentLoaded', (event) => {
    const colorWheel = document.getElementById('color_wheel');
    const colorPicker = document.getElementById('color_picker');
    const colorCanvas = document.getElementById('color_canvas');
    const ctx = colorCanvas.getContext('2d');

    colorWheel.addEventListener('click', (event) => {
        const rect = colorWheel.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;

        colorCanvas.width = colorWheel.width;
        colorCanvas.height = colorWheel.height;
        ctx.drawImage(colorWheel, 0, 0, colorCanvas.width, colorCanvas.height);

        const pixel = ctx.getImageData(x, y, 1, 1).data;
        const rgbColor = `rgb(${pixel[0]}, ${pixel[1]}, ${pixel[2]})`;
        // console.log(rgbColor);

        postRgbLed(pixel);
    });
});
