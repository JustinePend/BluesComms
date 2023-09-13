# Notecard-AWS Usage
Walkthrough of the setup and structure of the BluesComms project. This project was made for an internship, and some elements are private. This section of the project, a communication system to pass data from AWS to a main board using a BluesIO Notecard system, is good for public release.

The main board in this case was based on an ESP32-S2 Feather board. 

---
## Overview

This project uses a Blues Notecard (Notecarrier-F) as a communication module, so all of the data passes through Notehub. Notehub has a lot of resources for routing data to external services, but for demonstration purposes, we are using AWS as an example. 

Instructions for routing data to different cloud services are found here: 

https://dev.blues.io/guides-and-tutorials/routing-data-to-cloud/aws-iot-analytics/

Here is a crude diagram of the entire system:

![comms_diagram](https://github.com/JustinePend/BluesComms/assets/22806576/58fd6b4d-8447-45d7-b4c8-407c88508500)

---

## Setup 

This section will cover the full setup, starting with the Notecard. 

> ### Wiring

Make sure to set the small switch labeled 'DFE' to OFF. This will allow the F_TX and F_RX pins to be used. 
Wiring setup:

| Main Board | Notecard |
|:----------:|:---------|
| TX | F_RX |
| RX | F_TX | 
| GND | GND |

> ### Notehub

Follow the general instructions in this tutorial to set up a basic Notehub project:

https://dev.blues.io/guides-and-tutorials/collecting-sensor-data/notecarrier-f/adafruit-huzzah32/c-cpp-arduino-wiring/

(Select C/C++, HUZZAH32 Board, and Notecarrier-F)

An environment variable is used in the code for purposes of testing (setting the sensor reading inverval), so go ahead and set that up as well. In the code it is named "interval", but that can be changed. 

The code that is uploaded to the Notecard is found under firmware/platformIO/BluesComms. 

https://dev.blues.io/notecard/notecard-walkthrough/inbound-requests-and-shared-data/

> ### IOT Analytics

This next tutorial tutorial covers instructions for the next section, but we will modify them a bit (feel free to stop before the AWS Quicksight section, unless the intent is to use that service). Use this as a reference for the specifics. 

https://dev.blues.io/guides-and-tutorials/routing-data-to-cloud/aws-iot-analytics/

For using a different service, there is a good chance that it is included in the dropdown menu at the top of the page. Take a look at the tutorials for that instead. 

1. Get an AWS account and create an IAM user. Retrieve your access keys and save them.
2. Create a basic AWS IOT Analytics app
3. Create a route in Notehub. In the TransformJSON field, paste the following code as a JSONata Expression to reformat the data before it goes into AWS. The measurements should be whatever sensor data is being collected. 

```
{
  "device": {
    "deviceId": device
  },
  "measurements": {
    "temp": body.res.temp,
    "VOC" : body.res.voc,
    "speed" : body.res.speed
  },
  "timestamp": $fromMillis(when * 1000, "[M01]/[D01]/[Y0001] [H#1]:[m01]:[s01]")
}
```
4. Return to IOTAnalytics. In the datastore section, choose service-managed storage and JSON format.
5. In the pipelines section, the input will be our same channel, and the output will be the corresponding datastore we just made.
6. For each pipeline, click Edit on the Activities heading. This will take you to a page where you can select which data each pipeline covers.


7. In the dataset section, select SQL and set each corresponding datastore as the input. Remove the default `LIMIT 10` in the SQL query and set the frequency to whatever appropriate interval is desired. 


> ### AWS Lambda

To pass commands to the device, we're using an AWS Lambda function. This code doesn't require anything specific to AWS and can be transplanted into a script in a different service or just running on a local machine (with a few modifications). All it does is request an access token from Notehub and then send the JSON command that is passed into it to notehub. 

To set up the Lambda function itself, here is a simple overview: https://docs.aws.amazon.com/lambda/latest/dg/getting-started.html. 

The code can be found in Github under cloud_files. Four variables (project UID, device UID, client ID, client secret) are environment variables in the Lambda, and will need to be set. 

To find the `project UID`, head to the Notehub project. It's located under Settings, inside Project Information. 

To find the `device UID`, head to the Devices section and it is at the top.

To find the `client ID` and `client secret`, get programmatic API access following instructions on this page:

https://dev.blues.io/api-reference/notehub-api/api-introduction/


You can test the Lambda function by going the test tab and putting in a sample JSON command. The test should have two 200 return codes. 

If you wish to perform these functions manually without the lambda or testing code, the commands I used for testing were these:

Retrieving the Token from Notehub:
![get token](https://github.com/JustinePend/BluesComms/assets/22806576/91a8c9c3-43c2-4896-8747-2bdd2bf3fe20)

Sending a JSON to Notehub:
![send to notehub](https://github.com/JustinePend/BluesComms/assets/22806576/fba9874a-b3fb-456c-be26-7939f06ce791)

---

## Sending Commands

The communication module uses AWS as an example host for storing data on the cloud. Commands can be sent with a POST request to AWS.

First, generate a function URL for the Lambda by following these instructions: https://docs.aws.amazon.com/lambda/latest/dg/urls-configuration.html

Then, we can send a POST request with its body as the JSON expression that we want to send to the sensor. 
```
curl -X POST
     -L 'https://abcdefg.lambda-url.us-west-2.on.aws/'
     -H 'content-type: application/json'
     -d '{"cmd":"get","arg": {"param": "data"}}' 
```

Alternatively, use Postman.

![postman](https://github.com/JustinePend/BluesComms/assets/22806576/e021c715-1a8d-44ba-a9e9-aeefa68aa06f)

More information can be found here:

https://docs.aws.amazon.com/lambda/latest/dg/urls-invocation.html

>## JSON Commands

AWS is the user endpoint for both sending commands and displaying data. Commands are in JSON format. The project-specific commands aren't shareable, but as an example, I used them in a format like this:

```JSON
{
  "cmd": "get",
  "arg": {
    "param": "data"
  }
}
```

## Requesting Data

Data can be downloaded directly from AWS, sent to a visualization program such as AWS QuickSight, or requested directly at a url. 

We can download the data by using the Amazon CLI. Download and set it up here:

https://aws.amazon.com/cli/

https://docs.aws.amazon.com/cli/latest/userguide/getting-started-quickstart.html

Next, install jq (used for parsing JSON in the command line). 

https://jqlang.github.io/jq/download/

Once properly signed in, use this command to retrieve the data from a specified dataset:

`curl $(aws iotanalytics get-dataset-content --dataset-name data_dataset | jq .entries[].dataURI | tr -d '"')`

This will download the dataset as a csv file. You can filter specifics (ex. only getting data from the past day, etc) with SQL queries in the dataset section of IOTAnalytics. 

