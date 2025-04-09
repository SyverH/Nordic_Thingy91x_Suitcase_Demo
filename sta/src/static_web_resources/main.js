/*
 * Copyright (c) 2024, Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//////////////////////////////////////////////////////////////////
// map
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
// Setup the charts
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
            text: 'Angular velocity (deg/s)'
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
    const euler = quaternionToEuler(orientationQuat);

    plotOrientation(euler.roll, euler.pitch, euler.yaw);

}

function plotOrientation(roll, pitch, yaw) {

    // console.log("Roll: " + roll + " Pitch: " + pitch + " Yaw: " + yaw);

    const modelViewerTransform = document.querySelector("model-viewer#transform");
    modelViewerTransform.orientation = `${roll}deg ${pitch}deg ${yaw}deg`;
}

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

const fetchAttempts = 0;
const maxAttempts = 5;
const fetchInterval = 5000; // 5 seconds

function fetchLocation() {
    fetch("/location")
        .then(response => {
            if (!response.ok) {
                throw new Error(`Response status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            console.log(data);
            updateMarker(data.lat, data.lon, data.uncertainty, 15);
        })
        .catch(error => {
            console.error(error.message);
            if (fetchAttempts < maxAttempts) {
                setTimeout(() => {
                    fetchLocation();
                }, fetchInterval);
            }
        });

}

// Wait for the site to load before loading the 3D model
window.addEventListener('load', function () {
    var modelViewer = document.getElementById('transform');
    modelViewer.setAttribute('src', modelViewer.getAttribute('data-src'));
});

window.addEventListener("DOMContentLoaded", (ev) => {

    // document.querySelector('.color-picker-container img').addEventListener('click', function () {
    //     document.getElementById('color_picker').click();
    // });

    // document.getElementById('color_picker').addEventListener('input', function (event) {
    //     let color = event.target.value;
    //     postRgbLed(color);
    // });

    fetchLocation();

    document.getElementById('reset-orientation').addEventListener('click', function () {
        roll = 0.0;
        pitch = 0.0;
        yaw = 0.0;
        plotOrientation(roll, pitch, yaw);
    });

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
