/*
 * Copyright (c) 2024, Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */
let accel0_chart = new Highcharts.Chart({
    chart: {
        renderTo: 'chart_accel0'
    },
    title: {
        text: 'Accelerometer BMI270'
    },
    series: [{
        name: 'X',
        color: 'red',
        data: [], 
        marker: {enabled: false}
    }, {
        name: 'Y',
        color: 'green',
        data: [], 
        marker: {enabled: false}
    }, {
        name: 'Z',
        color: 'blue',
        data: [], 
        marker: {enabled: false}
    }],
    xAxis: {
        title: {
            text: 'Sample number'
        }
    },
    yAxis: {
        title: {
            text: 'Acceleration (m/s^2)'
        }
    }
});

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
//             text: 'Sample number'
//         }
//     },
//     yAxis: {
//         title: {
//             text: 'Acceleration (m/s^2)'
//         }
//     }
// });

let gyro0_chart = new Highcharts.Chart({
    chart: {
        renderTo: 'chart_gyro0'
    },
    title: {
        text: 'Gyroscope BMI270'
    },
    series: [{
        name: 'X',
        color: 'red',
        data: [], 
        marker: {enabled: false}
    }, {
        name: 'Y',
        color: 'green',
        data: [], 
        marker: {enabled: false}
    }, {
        name: 'Z',
        color: 'blue',
        data: [], 
        marker: {enabled: false}
    }],
    xAxis: {
        title: {
            text: 'Sample number'
        }
    },
    yAxis: {
        title: {
            text: 'Angular velocity (deg/s)'
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
        marker: {enabled: false}
    }],
    xAxis: {
        title: {
            text: 'Sample number'
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
        marker: {enabled: false}
    }],
    xAxis: {
        title: {
            text: 'Sample number'
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
        marker: {enabled: false}
    }],
    xAxis: {
        title: {
            text: 'Sample number'
        }
    },
    yAxis: {
        title: {
            text: 'Pressure (hPa)'
        }
    }
});

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
//             text: 'Sample number'
//         }
//     },
//     yAxis: {
//         title: {
//             text: 'Gas resistance (Ohm)'
//         }
//     }
// });



async function postLed(led_id, state)
{
	try {
		const payload = JSON.stringify({"led_num" : led_id, "led_state" : state});

		const response = await fetch("/led", {method : "POST", body : payload});
		if (!response.ok) {
			throw new Error(`Response satus: ${response.status}`);
		}
	} catch (error) {
		console.error(error.message);
	}
}

function setSensorData(json_data, sensor_name)
{
    document.getElementById(sensor_name).innerHTML = json_data[sensor_name];
}

window.addEventListener("DOMContentLoaded", (ev) => {

	/* POST to the LED endpoint when the buttons are pressed */
	const led_on_btn = document.getElementById("red_led_on");
	led_on_btn.addEventListener("click", (event) => {
		console.log("red_led_on clicked");
		postLed(0, true);
	})

	const led_off_btn = document.getElementById("red_led_off");
	led_off_btn.addEventListener("click", (event) => {
		console.log("red_led_off clicked");
		postLed(0, false);
	})

    const green_led_on_btn = document.getElementById("green_led_on");
    green_led_on_btn.addEventListener("click", (event) => {
        console.log("green_led_on clicked");
        postLed(1, true);
    })

    const green_led_off_btn = document.getElementById("green_led_off");
    green_led_off_btn.addEventListener("click", (event) => {
        console.log("green_led_off clicked");
        postLed(1, false);
    })

    const blue_led_on_btn = document.getElementById("blue_led_on");
    blue_led_on_btn.addEventListener("click", (event) => {
        console.log("blue_led_on clicked");
        postLed(2, true);
    })

    const blue_led_off_btn = document.getElementById("blue_led_off");
    blue_led_off_btn.addEventListener("click", (event) => {
        console.log("blue_led_off clicked");
        postLed(2, false);
    })

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

        // let x = (new Date()).getTime();
        let x = parseInt(data.count);
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

        // y = parseFloat(data.bme680_gas);
        // if (gas_chart.series[0].data.length > 1000) {
        //     gas_chart.series[0].addPoint([x, y], true, true, false);
        // } else {
        //     gas_chart.series[0].addPoint([x, y], true, false, false);
        // }
	}
})
