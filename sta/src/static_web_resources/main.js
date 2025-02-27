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


// NOTE: The accelerometer ADXL367 is not ploted in the web interface as this is the same data as the BMI270
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

let mag0_chart = new Highcharts.Chart({
    chart: {
        renderTo: 'chart_mag0'
    },
    title: {
        text: 'Magnetometer'
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
            text: 'Magnetic field (uT)'
        }
    }
});

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

// NOTE: The gas resistance is not ploted in the web interface as this value seems to be incorrect
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
    let x = parseFloat(data.count);
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

    //NOTE: The accelerometer ADXL367 is not ploted in the web interface as this is the same data as the BMI270
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

    y = parseFloat(data.bmm350_magn_x);
    if (mag0_chart.series[0].data.length > 100) {
        mag0_chart.series[0].addPoint([x, y], true, true, false);
    } else {
        mag0_chart.series[0].addPoint([x, y], true, false, false);
    }
    y = parseFloat(data.bmm350_magn_y);
    if (mag0_chart.series[1].data.length > 100) {
        mag0_chart.series[1].addPoint([x, y], true, true, false);
    } else {
        mag0_chart.series[1].addPoint([x, y], true, false, false);
    }
    y = parseFloat(data.bmm350_magn_z);
    if (mag0_chart.series[2].data.length > 100) {
        mag0_chart.series[2].addPoint([x, y], true, true, false);
    } else {
        mag0_chart.series[2].addPoint([x, y], true, false, false);
    }

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

    //NOTE: The gas resistance is not ploted in the web interface as this value seems to be incorrect
    // y = parseFloat(data.bme680_gas);
    // if (gas_chart.series[0].data.length > 1000) {
    //     gas_chart.series[0].addPoint([x, y], true, true, false);
    // } else {
    //     gas_chart.series[0].addPoint([x, y], true, false, false);
    // }
}


class KalmanFilter {
    constructor({ R = 1, Q = 1, A = 1, C = 1 } = {}) {

        this.R = R; // Process noise
        this.Q = Q; // Measurement noise

        this.A = A; // State vector
        this.C = C; // Measurement vector
        this.cov = NaN; // covariance
        this.x = NaN; // estimated signal without noise
    }
    filter(z) {

        if (isNaN(this.x)) {
            this.x = (1 / this.C) * z;
            this.cov = (1 / this.C) * this.Q * (1 / this.C);
        }
        else {

            // Compute prediction
            const predX = (this.A * this.x);
            const predCov = ((this.A * this.cov) * this.A) + this.R;

            // Kalman gain
            const K = predCov * this.C * (1 / ((this.C * predCov * this.C) + this.Q));

            // Correction
            this.x = predX + K * (z - (this.C * predX));
            this.cov = predCov - (K * this.C * predCov);
        }

        return this.x;
    }
}

function setSensorData(json_data, sensor_name) {
    document.getElementById(sensor_name).innerHTML = json_data[sensor_name];
}

function updateOrientation(roll, pitch, yaw) {
    // console.log("Roll: " + roll + " Pitch: " + pitch + " Yaw: " + yaw);

    const modelViewerTransform = document.querySelector("model-viewer#transform");

    modelViewerTransform.orientation = `${pitch}deg ${roll}deg ${yaw}deg`;
}

async function postRgbLed(hex_color) {
    let r = parseInt(hex_color.slice(1, 3), 16);
    let g = parseInt(hex_color.slice(3, 5), 16);
    let b = parseInt(hex_color.slice(5, 7), 16);

    try {
        const payload = JSON.stringify({ "r": r, "g": g, "b": b });

        const response = await fetch("/led", { method: "POST", body: payload });
        if (!response.ok) {
            throw new Error(`Response satus: ${response.status}`);
        }
    }
    catch (error) {
        console.error(error.message);
    }
}

async function postRecalibrate() {
    try {
        const response = await fetch("/recalibrate_gyro", { method: "POST" });
        if (!response.ok) {
            throw new Error(`Response satus: ${response.status}`);
        }
    }
    catch (error) {
        console.error(error.message);
    }
}

// Wait for the site to load before loading the 3D model
window.addEventListener('load', function () {
    var modelViewer = document.getElementById('transform');
    modelViewer.setAttribute('src', modelViewer.getAttribute('data-src'));
});

let old_time = 0;
let roll = 0.0;
let pitch = 0.0;
let yaw = 0.0;

var kf1 = new KalmanFilter({ R: 0.05, Q: 1.0 });
var kf2 = new KalmanFilter({ R: 0.05, Q: 1.0 });
var kf3 = new KalmanFilter({ R: 0.05, Q: 1.0 });

window.addEventListener("DOMContentLoaded", (ev) => {

    document.querySelector('.color-picker-container img').addEventListener('click', function () {
        document.getElementById('color_picker').click();
    });

    document.getElementById('color_picker').addEventListener('input', function (event) {
        let color = event.target.value;
        postRgbLed(color);
    });

    document.getElementById('reset-orientation-button').addEventListener('click', function () {
        roll = 0.0;
        pitch = 0.0;
        yaw = 0.0;
        updateOrientation(roll, pitch, yaw);
    });

    document.getElementById('recalibrate-offset-button').addEventListener('click', function () {
        postRecalibrate();
    });

    /* Setup websocket for handling network stats */
    const ws = new WebSocket("/");
    ws.onopen = (event) => {
        console.log("Connected to the server");
    }

    ws.onmessage = (event) => {
        // console.log("Received data");

        const data = JSON.parse(event.data);
        setSensorData(data, "adxl_ax");
        setSensorData(data, "adxl_ay");
        setSensorData(data, "adxl_az");

        setSensorData(data, "bme680_temperature");
        setSensorData(data, "bme680_humidity");
        setSensorData(data, "bme680_pressure");
        // setSensorData(data, "bme680_gas");

        setSensorData(data, "bmi270_gx");
        setSensorData(data, "bmi270_gy");
        setSensorData(data, "bmi270_gz");
        // setSensorData(data, "bmi270_ax");
        // setSensorData(data, "bmi270_ay");
        // setSensorData(data, "bmi270_az");

        setSensorData(data, "bmm350_magn_x");
        setSensorData(data, "bmm350_magn_y");
        setSensorData(data, "bmm350_magn_z");
        
        updatePlots(data);

        let time = parseFloat(data.count);
        if (old_time == 0) {
            old_time = time;
        }
        let delta_time = time - old_time;
        old_time = time;

        roll += delta_time * kf1.filter(parseFloat(data.bmi270_gx)) * 180 / Math.PI;
        pitch += delta_time * kf2.filter(parseFloat(data.bmi270_gy)) * 180 / Math.PI;
        yaw += delta_time * kf3.filter(parseFloat(data.bmi270_gz)) * 180 / Math.PI;

        // console.log("time: " + time + " delta time: " + delta_time + " roll: " + roll + " pitch: " + pitch + " yaw: " + yaw);
        console.log("gx: " + parseFloat(data.bmi270_gx) + " gy: " + parseFloat(data.bmi270_gy) + " gz: " + parseFloat(data.bmi270_gz));

        updateOrientation(roll, pitch, yaw);


    }
})
