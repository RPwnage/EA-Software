// Callback for SDK Blocked List Event handling
OriginErrorT BlockListEventCallback (int32_t eventId, void* pContext, void* eventData, uint32_t count)
{
	// Note: eventData is not used for Block List events as the event has no associated data.
	// This event indicates that the Block List has changed but does not say how.
	// To retrieve the most up to date block list a separate query has to be initiated.
	OriginHandleT handle;
	uint32_t TotalCount;

	OriginErrorT err = OriginQueryBlockedUsersSync(OriginGetDefaultUser(), &TotalCount, 10000, &handle);
	if (err == ORIGIN_SUCCESS && TotalCount)
	{
		OriginFriendT blocked[100];
		uint32_t read;

		err = OriginReadEnumeration(handle, blocked, sizeof(blocked), 0, ORIGIN_READ_ALL_DATA, &read);

		if (read)
		{
			for (uint32_t i=0; i<read; i++)
			{
				printf("Blocked User: %S(%I64u)", blocked[i].Persona, blocked[i].UserId);
			}
		}
	}

	OriginDestroyHandle(handle);

	return ORIGIN_SUCCESS;
}

// Changes to this user's block list can be received by registering for the block list event.
// This would normally be performed at start up, but is shown here for clarity.
void exampleBlockListEvent()
{
	OriginRegisterEventCallback(ORIGIN_EVENT_BLOCKED,  BlockListEventCallback, NULL);
}