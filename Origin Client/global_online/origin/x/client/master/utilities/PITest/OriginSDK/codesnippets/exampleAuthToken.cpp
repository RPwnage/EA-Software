

OriginErrorT TicketCallback(void * pContext, const OriginCharT **ticket, int32_t length, OriginErrorT err)
{
	if(err == ORIGIN_SUCCESS)
	{
		CUser *pMyUserClass = (CUser *)pContext;

		if(ticket != 0 && *ticket != 0)
		{
			pMyUserClass->myAuthToken = *ticket;

			// Call some function here to signal we received the auth token.
		}
		else
		{
			printf("Invalid Ticket\n");
		}
	}
	else
	{
		printf("Async: Getting an authtoken failed: (0x%08X, %s)", err, OriginGetErrorDescription(err));
	}
	return err;
}

void RequestMyAuthToken()
{
	OriginErrorT err = OriginRequestTicket(OriginGetDefaultUser(), TicketCallback, &myUserClass);
				
	if(err == ORIGIN_SUCCESS)
	{
		printf("Request send.\n");
	}
	else
	{
		printf("Async: Getting an authtoken failed: (0x%08X, %s)\n", err, OriginGetErrorDescription(err));
	}
}


