
std::string GetAuthToken()
{
	std::string myAuthToken;
	OriginHandleT ticket = 0;
	int size = 0;

	OriginErrorT err = OriginRequestTicketSync(OriginGetDefaultUser(), &ticket, &size);

	if(err == ORIGIN_SUCCESS)
	{
		if(ticket != NULL && size != 0)
		{
			myAuthToken = (LPCSTR)ticket;
		}
		else
		{
			printf("Invalid Ticket");
		}

		OriginDestroyHandle(ticket);
	}
	else
	{
		printf("Sync: Getting an authtoken failed: (0x%08X, %s)", err, OriginGetErrorDescription(err));
	}

	return myAuthToken;
}
