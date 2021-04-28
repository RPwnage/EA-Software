// Check for the visibility API
if ((typeof(document.hidden) === "undefined") && window.originPageVisibility)
{
	document.__defineGetter__("hidden", function()
	{
		return window.originPageVisibility.hidden;
	});

	document.__defineGetter__("visibilityState", function()
	{
		return window.originPageVisibility.visibilityState;
	});

	window.originPageVisibility.visibilityChanged.connect(function()
	{
		var visibilityEvent = document.createEvent('CustomEvent');

		visibilityEvent.initCustomEvent('visibilitychange', false, false, null);
		document.dispatchEvent(visibilityEvent);
	});
}
