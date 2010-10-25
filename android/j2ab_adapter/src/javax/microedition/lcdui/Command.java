/*
 * Copyright (C) 2009 The J2AB Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package javax.microedition.lcdui;

public class Command {
    public static final int SCREEN = 1;
    public static final int BACK   = 2;
    public static final int CANCEL = 3;
    public static final int OK     = 4;
    public static final int HELP   = 5;
    public static final int STOP   = 6;
    public static final int EXIT   = 7;
    public static final int ITEM   = 8;

    private int priority;
    private int commandType;
    private String label;
    private String longLabel;

    public Command(String label, int commandType, int priority) {
        this (label, null, commandType, priority);
    }

    public Command(String label, String longLabel, int commandType, int priority) {
        this.label = label;
        this.longLabel = longLabel;
        this.commandType = commandType;
        this.priority = priority;
    }

    public int getCommandType () {
        return commandType;
    }

    public String getLabel () {
        return label;
    }

    public int getPriority () {
        return priority;
    }
}
