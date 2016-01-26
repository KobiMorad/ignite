/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.ignite.mesos.resource;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.channels.Channels;
import org.apache.ignite.mesos.ClusterProperties;

/**
 * Class downloads and stores Ignite.
 */
public class IgniteProvider {
    /** */
    public static final String DOWNLOAD_LINK = "http://www.ignite.run/download_ignite.php";

    /** */
    public static final String DOWNLOAD_URL_PATTERN =
        "http://www.us.apache.org/dist/ignite/%s/apache-ignite-fabric-%s-bin.zip";

    /** */
    private String downloadFolder;

    /**
     * @param downloadFolder Folder with ignite.
     */
    public IgniteProvider(String downloadFolder) {
        this.downloadFolder = downloadFolder;
    }

    /**
     * @param ver Ignite version.
     * @throws IOException If downloading failed.
     * @return Path to latest ignite.
     */
    public String getIgnite(String ver) throws IOException {
        return downloadIgnite(ver);
    }

    /**
     * @param ver Ignite version which will be downloaded. If {@code null} will download the latest ignite version.
     * @throws IOException If downloading failed.
     * @return Ignite archive.
     */
    public String downloadIgnite(String ver) throws IOException {
        URL url;

        if (ver.equals(ClusterProperties.DEFAULT_IGNITE_VERSION)) {
            URL updateUrl = new URL(DOWNLOAD_LINK);

            HttpURLConnection conn = (HttpURLConnection)updateUrl.openConnection();

            int code = conn.getResponseCode();

            if (code == 200)
                url = conn.getURL();
            else
                throw new RuntimeException("Failed. Got unexpected response code. Response code: " + code);
        }
        else
            url = new URL(String.format(DOWNLOAD_URL_PATTERN, ver.replace("-incubating", ""), ver));

        return downloadIgnite(url);
    }

    /**
     * Downloads ignite by URL if this version wasn't downloaded before.
     *
     * @param url URL to Ignite.
     * @return File name in
     */
    private String downloadIgnite(URL url) {
        assert url != null;

        try {
            HttpURLConnection conn = (HttpURLConnection)url.openConnection();

            int code = conn.getResponseCode();

            if (code == 200) {
                checkDownloadFolder();

                String fileName = fileName(url.toString());

                if (fileExist(fileName))
                    return fileName;

                FileOutputStream outFile = new FileOutputStream(downloadFolder + fileName);

                outFile.getChannel().transferFrom(Channels.newChannel(conn.getInputStream()), 0, Long.MAX_VALUE);

                outFile.close();

                return fileName;
            }
            else
                throw new RuntimeException("Got unexpected response code. Response code: " + code);
        }
        catch (IOException e) {
            throw new RuntimeException("Failed to download Ignite.", e);
        }
    }

    /**
     * Checks that file exists.
     *
     * @param fileName File name.
     * @return {@code True} if file exist otherwise {@code false}.
     */
    public boolean fileExist(String fileName) {
        String pathToIgnite = downloadFolder + (downloadFolder.endsWith("/") ? "" : '/') + fileName;

        return new File(pathToIgnite).exists();
    }

    /**
     * @return Download folder.
     */
    private File checkDownloadFolder() {
        File file = new File(downloadFolder);

        if (!file.exists())
            file.mkdirs();

        if (!file.exists())
            throw new IllegalArgumentException("Failed to create working directory: " + downloadFolder);

        return file;
    }

    /**
     * @param url URL.
     * @return File name.
     */
    private static String fileName(String url) {
        String[] split = url.split("/");

        return split[split.length - 1];
    }
}