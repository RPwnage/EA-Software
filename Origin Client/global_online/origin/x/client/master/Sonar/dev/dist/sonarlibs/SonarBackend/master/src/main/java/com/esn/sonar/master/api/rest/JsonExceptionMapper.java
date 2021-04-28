package com.esn.sonar.master.api.rest;


import org.codehaus.jackson.JsonParseException;

import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.ext.ExceptionMapper;
import javax.ws.rs.ext.Provider;

@Provider
public class JsonExceptionMapper implements ExceptionMapper<JsonParseException>
{
    private static class Message
    {
        public String message;
    }

    public Response toResponse(JsonParseException exception)
    {
        Message msg = new Message();
        msg.message = "Request body not valid JSON";
        return Response.status(Response.Status.BAD_REQUEST).type(MediaType.APPLICATION_JSON).entity(msg).build();
    }
}

