// In this example we'll retrieve the weather in Las Vegas using
// Google's API.  The response is in XML which could be processed
// and used by the game using SimXMLDocument, but we'll just output
// the results to the console in this example.

// Define callbacks for our specific HTTPObject using our instance's
// name (WeatherFeed) as the namespace.

// Handle an issue with resolving the server's name
function WeatherFeed::onDNSFailed(%this)
{
   // Store this state
   %this.lastState = "DNSFailed";

   // Handle DNS failure
}

function WeatherFeed::onConnectFailed(%this)
{
   // Store this state
   %this.lastState = "ConnectFailed";

   // Handle connection failure
}

function WeatherFeed::onDNSResolved(%this)
{
   // Store this state
   %this.lastState = "DNSResolved";

}

function WeatherFeed::onConnected(%this)
{
   // Store this state
   %this.lastState = "Connected";

   // Clear our buffer
   %this.buffer = "";
}

function WeatherFeed::onDisconnect(%this)
{
   // Store this state
   %this.lastState = "Disconnected";

   // Output the buffer to the console
   echo("Google Weather Results:");
   echo(%this.buffer);
}

// Handle a line from the server
function WeatherFeed::onLine(%this, %line)
{
   // Store this line in out buffer
   %this.buffer = %this.buffer @ %line;
}

function testHTTPObject()
{
	// Create the HTTPObject
	if(!isObject($feed))
		$feed = new HTTPObject(WeatherFeed);

	// Define a dynamic field to store the last connection state
	$feed.lastState = "None";

	// Send the GET command
	$feed.get("www.google.com:80", "/ig/api", "weather=Las-Vegas,US");
}