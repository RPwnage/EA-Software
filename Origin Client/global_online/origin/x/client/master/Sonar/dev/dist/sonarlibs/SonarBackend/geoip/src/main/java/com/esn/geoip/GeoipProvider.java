package com.esn.geoip;

import java.io.File;
import java.io.IOException;

public interface GeoipProvider
{

    public File[] getFiles(String basePath);

    public void loadFromJar(Class cls, String basePath) throws IOException;

    public void loadFromDisk(String basePath) throws IOException;

    public Position getPosition(String address);

    public String getLicense();
}
