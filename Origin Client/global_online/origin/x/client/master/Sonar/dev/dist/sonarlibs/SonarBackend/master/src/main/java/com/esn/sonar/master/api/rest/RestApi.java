package com.esn.sonar.master.api.rest;

import com.esn.sonar.core.Protocol;
import com.esn.sonar.master.api.operator.OperatorService;

import javax.ws.rs.*;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import java.util.Arrays;
import java.util.Collection;

@Path("/api/{operatorId}")
@Produces(MediaType.APPLICATION_JSON)
public class RestApi
{
    static private OperatorService operatorService;

    public static void setOperatorService(OperatorService operatorService)
    {
        RestApi.operatorService = operatorService;
    }

    public class NotFoundException extends WebApplicationException
    {
        public NotFoundException(Throwable cause)
        {
            super(cause, Response.Status.NOT_FOUND);
        }
    }

    public class UnavailableException extends WebApplicationException
    {
        public UnavailableException(Throwable cause)
        {
            super(cause, Response.Status.SERVICE_UNAVAILABLE);
        }
    }

    public class BadRequestException extends WebApplicationException
    {
        public BadRequestException(Throwable cause)
        {
            super(cause, Response.Status.BAD_REQUEST);
        }
    }


    @PathParam("operatorId")
    private String operatorId;

    public RestApi()
    {
    }

    /**
     * Generates a token that a user can use to connect to the voice server
     *
     * @param userId   User ID to generate token for
     * @param userDesc User Description to generate token for
     * @return Generated token
     */
    @GET
    @Path("/users/{userId}/channel-token")
    public String getChannelToken(@PathParam("userId") String userId,
                                  @QueryParam("udc") String userDesc,
                                  @QueryParam("cid") String channelId,
                                  @QueryParam("cdc") String channelDesc)
    {
        if (userDesc == null)
            userDesc = userId;

        if (channelId == null)
            channelId = "";

        if (channelDesc == null)
            channelDesc = "";

        try
        {
            return operatorService.getChannelToken(operatorId, userId, userDesc, channelId, channelDesc, "", "");
        } catch (OperatorService.InvalidArgumentException e)
        {
            throw new BadRequestException(e);
        } catch (OperatorService.ChannelAllocationFailed e)
        {
            throw new UnavailableException(e);
        }
    }

    /**
     * Generates a token that a user can use to connect to the control server
     *
     * @param userId   User ID to generate token for
     * @param userDesc User Description to generate token for
     * @return Generated token
     */
    @GET
    @Path("/users/{userId}/control-token")
    public String getUserControlToken(@PathParam("userId") String userId,
                                      @QueryParam("udc") String userDesc,
                                      @QueryParam("cid") String channelId,
                                      @QueryParam("cdc") String channelDesc)
    {
        if (userDesc == null)
            userDesc = userId;

        if (channelId == null)
            channelId = "";

        if (channelDesc == null)
            channelDesc = "";

        try
        {
            return operatorService.getUserControlToken(operatorId, userId, userDesc, channelId, channelDesc, "");
        } catch (OperatorService.InvalidArgumentException e)
        {
            throw new BadRequestException(e);
        } catch (OperatorService.UnavailableException e)
        {
            throw new UnavailableException(e);
        }
    }

    /**
     * Instruct a connected user to join a specific voice channel
     *
     * @param userId      User to instruct
     * @param channelId   Id for channel to join
     * @param channelDesc Description for channel to join (meta)
     * @param location    Location identifier for channel to join
     * @return The requested channel to join
     */
    @PUT
    @Path("/users/{userId}/channel")
    public Object joinUserToChannel(String channelId, @PathParam("userId") String userId, @QueryParam("location") String location, @QueryParam("channelDesc") String channelDesc)
    {
        if (location == null)
            location = "";

        if (channelDesc == null)
            channelDesc = "";

        try
        {
            operatorService.joinUserToChannel(operatorId, location, userId, channelId, channelDesc);
        } catch (OperatorService.ChannelAllocationFailed e)
        {
            return Response.status(Response.Status.SERVICE_UNAVAILABLE).entity(e.getMessage()).type(MediaType.APPLICATION_JSON).build();
        } catch (OperatorService.InvalidArgumentException e)
        {
            throw new BadRequestException(e);
        }
        //clientSetChannel
        // Return 202 Accepted
        return channelId;
    }

    /**
     * Instruct a connected user to leave a specific voice channel
     *
     * @param userId     User to instruct
     * @param channelId  Id of channel to leave, leave empty to make user leave any channel
     * @param reasonType Reason type identifier (meta)
     * @param reasonDesc Reason description identifier (
     * @return The requested channel to leave
     */
    @DELETE
    @Path("/users/{userId}/channel/{channelId}")
    public String partUserFromChannel(@PathParam("userId") String userId, @PathParam("channelId") String channelId, @QueryParam("reasonType") String reasonType, @QueryParam("reasonDesc") String reasonDesc)
    {
        if (reasonType == null)
            reasonType = Protocol.Reasons.Unknown;
        if (reasonDesc == null)
            reasonDesc = "";

        try
        {
            operatorService.partUserFromChannel(operatorId, userId, channelId, reasonType, reasonDesc);
        } catch (OperatorService.ChannelNotFoundException e)
        {
            throw new NotFoundException(e);
        } catch (OperatorService.OutOfSyncException e)
        {
            throw new NotFoundException(e);
        } catch (OperatorService.UserNotFoundException e)
        {
            throw new NotFoundException(e);
        } catch (OperatorService.NotInThatChannelException e)
        {
            throw new NotFoundException(e);
        } catch (OperatorService.InvalidArgumentException e)
        {
            throw new BadRequestException(e);
        }

        // Return 202 Accepted
        return channelId;
    }

    /**
     * Remove a voice channel, kicking all users in it
     *
     * @param channelId  Channel to remove
     * @param reasonType Reason type meta
     * @param reasonDesc Reason description meta
     * @return The requested channel to leave
     */
    @DELETE
    @Path("/channels/{channelId}")
    public String destroyChannel(@PathParam("channelId") String channelId, @QueryParam("reasonType") String reasonType, @QueryParam("reasonDesc") String reasonDesc)
    {
        if (reasonType == null)
            reasonType = "";
        if (reasonDesc == null)
            reasonDesc = "";

        try
        {
            operatorService.destroyChannel(operatorId, channelId, reasonType, reasonDesc);
        } catch (OperatorService.ChannelNotFoundException e)
        {
            throw new NotFoundException(e);
        } catch (OperatorService.InvalidArgumentException e)
        {
            throw new BadRequestException(e);
        }

        // Return 202 Accepted
        return channelId;
    }

    @GET
    @Path("/statistics/")
    public String getStatistics()
    {
        return "";
    }

    /**
     * Queries a user for it's online status.
     *
     * @param userId Id of user to query for online status
     * @return UserOnlineStatus
     */
    @GET
    @Path("/users/{userId}")
    public OperatorService.UserOnlineStatus getUserOnlineStatus(@PathParam("userId") String userId)
    {
        try
        {
            return operatorService.getUsersOnlineStatus(operatorId, Arrays.asList(userId)).iterator().next();
        } catch (OperatorService.InvalidArgumentException e)
        {
            throw new BadRequestException(e);
        }
    }

    /**
     * Retreives the userIds  a voice channel, kicking all users in it
     *
     * @param channelId Id of channel
     * @return The requested channel to leave
     */
    @GET
    @Path("/channels/{channelId}")
    public Collection<String> getUsersInChannel(@PathParam("channelId") String channelId)
    {
        try
        {
            return operatorService.getUsersInChannel(operatorId, channelId);
        } catch (OperatorService.ChannelNotFoundException e)
        {
            throw new NotFoundException(e);
        } catch (OperatorService.InvalidArgumentException e)
        {
            throw new BadRequestException(e);
        }
    }
}
