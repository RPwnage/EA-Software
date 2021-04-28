void exampleBlockList()
{
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
}