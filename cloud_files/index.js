//This is a script that operates inside an AWS lambda function.
//It can be modified to be used elsewhere, but the environment variables for authentication should be changed based on the user. 
//The first POST request gets an access token from BluesIO.
//The second uses that token to pass a JSON command (coming in as the input from an event) to Notehub.  

const https = require('https');
const qs = require('querystring');

const project_uid = process.env.project_uid;
const device_uid = process.env.device_uid;
const client_id = process.env.client_id;
const client_secret = process.env.client_secret;

var access_token;

//Request access token
const doTokenPostRequest = (event) => {
  return new Promise((resolve, reject) => {
    const token_options = {
      host: 'notehub.io',
      path: '/oauth2/token',
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded'
      }, 
      maxRedirects: 20
    };
    
    const req = https.request(token_options, (res) => {
      resolve(JSON.stringify(res.statusCode));
      res.on('data', (chunk) => {
        console.log(`BODY: ${chunk}`);
        access_token = JSON.parse(chunk).access_token;
      });
    });
    
    req.on('err', (e) => {
      reject(e.message);
    });
    
    var postData = qs.stringify({
      'grant_type': 'client_credentials',
      'client_id': client_id,
      'client_secret': client_secret
    });
    
    req.write(postData);
  
    req.end();
  });
};


//Send command to Notehub
const doNotePostRequest = (event) => {
    return new Promise((resolve, reject) => {
        const options = {
          host: 'api.notefile.net',
          path: '/v1/projects/' + project_uid + '/devices/' + device_uid + '/notes/sensors.qis',
          method: 'POST',
          headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
            'Authorization': 'Bearer ' + access_token
          }
        };
        
        //create the request object with the callback with the result
        const req = https.request(options, (res) => {
            resolve(JSON.stringify(res.statusCode));
        });
    
        // handle the possible errors
        req.on('err', (e) => {
          reject(e.message);
        });

        //do the request
        console.log(JSON.stringify(event));
        req.write('{"body": ' + JSON.stringify(event) + '}');
       

        //finish the request
        req.end();
        
    });
};

exports.handler = async (event, context) => {
  var body = JSON.parse(event['body']); //Comes in from POST request in a convoluted format, pare down to the relevant info here. 
  await doTokenPostRequest(body)
    .then(result => console.log(`Status code: ${result}`))
    .catch(err => console.error(`Error doing the request for token: ${JSON.stringify(body)} => ${err}`));
    
  await doNotePostRequest(body)
    .then(result => console.log(`Status code: ${result}`))
    .catch(err => console.error(`Error doing the request for notehub: ${JSON.stringify(body)} => ${err}`));
};
