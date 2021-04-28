package com.ea.originx.automation.scripts;

import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.SysEnvConstant;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.testng.annotations.Factory;

public class OAStressTestFactory {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Allows stress test to repeat one test case with given repeat count from
     * context concurrently.
     *
     * @return A matrix with test case instance to repeat
     * @throws java.lang.Exception
     */
    @Factory
    public Object[] factoryMethod() throws Exception {

        final String className = OSInfo.getEnvValue(SysEnvConstant.ENV_KEY_REPEAT_CASE);
        if (null != className && !className.isEmpty()) {
            _log.debug("stress repeat case: " + className);
            final Constructor<?> ctor;
            try {
                final Class<?> clazz = Class.forName(className);
                ctor = clazz.getConstructor();
            } catch (ClassNotFoundException | NoSuchMethodException | SecurityException e) {
                _log.error(e, e);
                throw e;
            }

            final int stressRepeatCount;
            final String value = OSInfo.getEnvValue(SysEnvConstant.ENV_KEY_REPEAT_COUNT);
            if (null != value && !value.isEmpty()) {
                _log.debug("stress repeat count: " + value);
                try {
                    stressRepeatCount = Integer.parseInt(value);
                } catch (NumberFormatException e) {
                    _log.error(e, e);
                    throw e;
                }
            } else {
                _log.error("cannot find '" + SysEnvConstant.ENV_KEY_REPEAT_COUNT + "' environment variable.");
                return new Object[0];
            }
            final Object[] matrix = new Object[stressRepeatCount];
            try {
                for (int i = 0; i < stressRepeatCount; ++i) {
                    matrix[i] = ctor.newInstance();
                }
            } catch (InstantiationException | IllegalAccessException | InvocationTargetException e) {
                _log.error(e, e);
                throw e;
            }
            _log.info("stress repeat case '" + className + "' " + stressRepeatCount + " times");
            return matrix;
        } else {
            _log.error("cannot find '" + SysEnvConstant.ENV_KEY_REPEAT_CASE + "' environment variable.");
            return new Object[0];
        }
    }

}
