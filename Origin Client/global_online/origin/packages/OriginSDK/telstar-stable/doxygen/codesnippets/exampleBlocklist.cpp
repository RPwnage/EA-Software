OriginErrorT QueryBlockedUsersCallbackFunc(void *pContext, OriginHandleT hHandle, uint32_t total, uint32_t available, OriginErrorT err);
void exampleBlockList()
{
	// Query which users this user has blocked. The results are given in the callback function below.
	OriginHandleT handle;
	OriginQueryBlockedUsers(OriginGetDefaultUser(), QueryBlockedUsersCallbackFunc, NULL, 10000, &handle);
}

OriginErrorT QueryBlockedUsersCallbackFunc(void *pContext, OriginHandleT hHandle, uint32_t total, uint32_t available, OriginErrorT err)
{
	OriginErrorT local_err = err;

	if (err == ORIGIN_SUCCESS)
	{
		OriginFriendT blocked[100];
		uint32_t read;

		local_err = OriginReadEnumeration(hHandle, blocked, sizeof(blocked), 0, ORIGIN_READ_ALL_DATA, &read);

		if (read)
		{
			for (uint32_t i=0; i<read; i++)
			{
				printf("Blocked Users: User: %S(%I64u)", blocked[i].Persona, blocked[i].UserId);
			}
		}
	}

	err = OriginDestroyHandle(hHandle);

	return local_err;
}