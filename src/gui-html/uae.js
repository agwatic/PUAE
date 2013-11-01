// Helper functions used throughout.

function $(element) {
  return document.getElementById(element);
}

function mimeHandlerExists (mimeType) {
	supportedTypes = navigator.mimeTypes;
	for (var i = 0; i < supportedTypes.length; i++) {
		if (supportedTypes[i].type == mimeType) {
			return true;
		}
	}
	return false;
}

function updateStatus(opt_message) {
  var statusField = document.getElementById('status_field');
  if (statusField) {
    statusField.innerHTML = statusField.innerHTML + "<br/>" + opt_message;
  }
}

function logMessage(message) {
  console.log(message);
}

if (!mimeHandlerExists('application/x-pnacl')) {
	updateStatus('This page uses <a href="https://developers.google.com/native-client/pnacl-preview/">Portable Native Client</a>, a technology currently only supported in Google Chrome (version 31 or higher; Android and iOS not yet supported).');
	document.getElementById('progress')
		.style.visibility='hidden';
}




module = null;  // Global application object.

window.onbeforeunload = function () {
    // TODO(cstefansen) return 'If you leave this page, the state of your game will be lost.';
}


// These event handling functions are were taken from the NaCl SDK
// examples and slightly adjusted.  

function moduleDidStartLoad() {
  logMessage('loadstart');
}

var statusMessages = [
	"reticulating splines",
	"calling the 80s",
	"polishing floppy disks",
	"finding Chuck Norris",
	"removing guru meditations",
	"preparing garden gnomes",
	"assembling floppy drive",
	"waking up Agnus, Denise, and Paula",
	"loading up Portable Native Client",
	"unpacking Amiga 500",
	"calibrating non-interlaced mode",
	"warming up the Motorola 68000"
	]
var currentMessage = 0;
var intervalUpdate = setInterval(
  function() {
      currentMessage = (currentMessage + 1) % statusMessages.length;
      document.getElementById('progress_text').innerHTML =
        'Loading... (' + statusMessages[currentMessage] + ')'; 
  }, 2000);

var lastLoadPercent = 0;

function moduleLoadProgress(event) {
  var loadPercent = 0.0;
  var loadPercentString;
  var loadStatus;
  
  if (event.lengthComputable && event.total > 0) {
	clearInterval(intervalUpdate);
    loadPercent = (event.loaded / event.total * 100.0).toFixed(0);
    if (loadPercent == lastLoadPercent) return;
    lastLoadPercent = loadPercent;  
    loadPercentString = loadPercent + '%';
    logMessage('progress: ' + event.url + ' ' + loadPercentString +
                     ' (' + event.loaded + ' of ' + event.total + ' bytes)');
	document.getElementById('progress_text').innerHTML =
	  'Loading... (' + loadPercentString + ')'; 
  } else {
    // The total length is not yet known.
    logMessage('progress: Computing...');
  }
}

// Handler that gets called if an error occurred while loading the NaCl
// module.  Note that the event does not carry any meaningful data about
// the error, you have to check lastError on the <EMBED> element to find
// out what happened.
function moduleLoadError() {
  logMessage('error: ' + module.lastError);
  updateStatus('Error: ' + module.lastError);
}

// Handler that gets called if the NaCl module load is aborted.
function moduleLoadAbort() {
  logMessage('abort');
}

    // Indicate success when the NaCl module has loaded.
   function moduleDidLoad() {
      module = document.getElementById('uae');
      // Set up general postMessage handler
      module.addEventListener('message', handleMessage, true);

      // Set up handler for kickstart ROM upload field
      document.getElementById('rom').addEventListener(
      	'change', handleFileSelect, false);
      // Set up handler for disk drive 0 (df0:) upload field
      document.getElementById('df0').addEventListener(
      	'change', handleFileSelect, false);
      // Set up handler for disk drive 1 (df1:) upload field
      document.getElementById('df1').addEventListener(
      	'change', handleFileSelect, false);

      document.getElementById('resetBtn')
        .addEventListener('click', resetAmiga, true);
      document.getElementById('eject0Btn')
        .addEventListener('click', function(e) { ejectDisk('df0'); }, true);
      document.getElementById('eject1Btn')
        .addEventListener('click', function(e) { ejectDisk('df1'); }, true);
      document.getElementById('port0').addEventListener('change',
	    function(e) {
	        connectJoyPort('port0',
	                   e.target.options[e.target.selectedIndex].value); },
	    true);
      document.getElementById('port1').addEventListener('change',
	    function(e) {
	        connectJoyPort('port1',
	                   e.target.options[e.target.selectedIndex].value); },
	    true);

      document.plugins.uae.focus();
      logMessage("Module loaded.");
    }

function moduleDidEndLoad() {
  clearInterval(intervalUpdate);
  document.getElementById('overlay')
      	.style.visibility='hidden'
  logMessage('loadend');
  var lastError = event.target.lastError;
  if (lastError == undefined || lastError.length == 0) {
    lastError = '<none>';

	// If we loaded without errors, we can activate the buttons.
	// Would have been logical to do this in moduleDidLoad, but in Chrome
	// 32, there is an initial moduleDidLoad event when PNaCl starts loading
	// itself, which is way before the emulator gets started.
    $('rom').disabled = false;
	$('df0').disabled = false;
	$('eject0Btn').disabled = false;
	$('df1').disabled = false;
	$('eject1Btn').disabled = false;
	$('resetBtn').disabled = false;
	$('port0').disabled = false;
	$('port1').disabled = false;
  }
  logMessage('lastError: ' + lastError);
}

    // TODO(cstefansen): Connect module's stdout/stderr to this.
    function handleMessage(message_event) {
        logMessage(message_event.data);
    }

    function resetAmiga() {
    	module.postMessage('reset');
	}

    // Ejects the disk in the current drive (e.g. 'df0', 'df1'). If there
    // is no disk in the drive, no action is taken.
    function ejectDisk(driveDescriptor) {
    	switch (driveDescriptor) {
    		case 'df0':
    		case 'df1':
    			module.postMessage('eject ' + driveDescriptor);
                        document.getElementById(driveDescriptor).value = "";
                        break;
			default:
				alert('Internal page error. Try to reload the page. If the ' +
					  'problem persists, please report the issue.');
		}
    }

	function connectJoyPort(portId, inputDevice) {
		module.postMessage('connect ' + portId + ' ' + inputDevice);
	}

	// Load a file to targetDevice, a pseudo-device, which can be 'rom',
	// 'df0', or 'df1'.
	function loadFromLocal(targetDevice, file) {
  	    var fileURL = window.webkitURL.createObjectURL(file);
    	switch (targetDevice) {
    		case 'df0':
    		case 'df1':
  	    		module.postMessage('insert ' + targetDevice + ' ' + fileURL);
  	    		break;
    		case 'rom':
  	    		module.postMessage('rom ' + fileURL);
  	    		break;
    		default:
    			alert('Internal page error. Try to reload the page. If the ' +
					  'problem persists, please report the issue.');
		}
	}

	// This functions is called when a kickstart ROM or a disk image is
	// uploaded via the input fields on the page.
	function handleFileSelect(evt) {
  		loadFromLocal(evt.target.id, evt.target.files[0]);
	}


var container = document.getElementById('uae_container');

container.addEventListener('loadstart', moduleDidStartLoad, true);
container.addEventListener('progress', moduleLoadProgress, true);
container.addEventListener('error', moduleLoadError, true);
container.addEventListener('abort', moduleLoadAbort, true);
container.addEventListener('load', moduleDidLoad, true);
container.addEventListener('loadend', moduleDidEndLoad, true);
container.addEventListener('message', handleMessage, true);
