EagleMQ
========

EagleMQ is an open source, high-performance and lightweight queue manager.

Basic commands
===============
* .auth
* .ping
* .stat
* .save
* .flush
* .disconnect

Description of basic commands
=============================
.auth(name, password)
---------------------------------
Command *.auth* is used to authenticate the client on the server.

*name* - the user name

*password* - the user password

Username *name* and password *password* can not have a length greater than 32.

.ping
--------
Command *.ping* send ping message to the server and receive a response.
Used to check the server status.

To run this command, no authentication is required on the server.

.stat
-------
Command *.stat* is used to get statistics server.

The server provides the following statistics:

* Version major, minor, patch - information about the server version
* Uptime - time in seconds since the start
* Used CPU [system, user] - CPU usage
* Used [memory, memory RSS] - memory usage
* Memory fragmentation ratio - information about the level of memory fragmentation
* Clients - number of connected clients
* Users - the number of users
* Queues - number of queues

.save(async)
------------
Command *.save* is used to save data to disk.

*async* sets the mode of use.

If *async* is 0 - use asynchronous data saving.

If *async* is 1 - use asynchronous data saving.

.flush(flags)
----------------
Command *.flush* is used to force deletion the data from the server.

Flags *flags* are a bit sequence.
Flags *flags* may take two values - FLUSH\_USER or FLUSH\_QUEUE.

Flag FLUSH\_USER indicates that deletes all users (except the main administrator).

Flag FLUSH\_QUEUE indicates that deletes all queues.

Recommended to use this command only when you need to delete all the data from the server.

.disconnect
-----------------
Command *.disconnect* disconnects the client from the server.

To run this command, no authentication is required on the server.

Users
============
EagleMQ is a multiuser system and allows for an unlimited number of users.
Users are used to constrain the authenticated client in order to secure the data.
For example you can create a user who will have the right only to create and delete queues.
Using the users is a good practice in the development of multi-component systems where each component performs a specific task on the server.
The default user name and password - eagle/eagle


Commands for working with users
====================================
* .user\_create
* .user\_list
* .user\_rename
* .user\_set\_perm
* .user\_delete

Description of commands for working with users
===============================================
.user\_create(name, password, perm)
-----------------------------------------------------
Command *.user\_create* creates a user named *name*, with password *password* and permissions *perm*.

Permissions *perm* are a bit sequence. Supported user permissions, see Table 1.

Username *name* and password *password* can not have a length greater than 32.

.user\_list
-------------
Command *.user\_list* used to get a list of users.

The server provides the following information about a user:

* name - the user name
* password - the user password
* permissions - the user permissions

.user\_rename(from, to)
----------------------------------
Command *.user\_rename* renames a user named *from* in *to*.

Username *from* and *to* can not have a length greater than 32.

.user\_set\_perm(name, perm)
--------------------------------------------
Command *.user\_set\_perm* sets a new permissions *perm* to user with name *name*.

New permissions take effect after re-authentication.

Username *name* can not have a length greater than 32.

.user\_delete(name)
----------------------------
Command *.user\_delete* deletes a user named *name*.

Username *name* can not have a length greater than 32.

Table 1. Description of user permissions
-------------------------------------------------------------
<table border="1">
    <tr>
        <td><b>№</b></td>
        <td><b>Name</b></td>
        <td><b>Bit number</b></td>
        <td><b>Command</b></td>
        <td><b>Description</b></td>
    </tr>
    <tr>
        <td>1</td>
        <td>QUEUE_PERM</td>
        <td>0</td>
        <td>
            <i>.queue_create</i><br/>
            <i>.queue_declare</i><br/>
            <i>.queue_exist</i><br/>
            <i>.queue_list</i><br/>
            <i>.queue_size</i><br/>
            <i>.queue_push</i><br/>
            <i>.queue_get</i><br/>
            <i>.queue_pop</i><br/>
            <i>.queue_subscribe</i><br/>
            <i>.queue_unsubscribe</i><br/>
            <i>.queue_purge</i><br/>
            <i>.queue_delete</i>
		</td>
        <td>The ability to perform all the commands associated with queues</td>
    </tr>
    <tr>
        <td>2</td>
        <td>ADMIN_PERM</td>
        <td>5</td>
        <td>-</td>
        <td>The ability to perform all the commands on the server (including the commands for working with users)</td>
    </tr>
    <tr>
        <td>3</td>
        <td>NOT_CHANGE_PERM</td>
        <td>6</td>
        <td>-</td>
        <td>If a user is created with the flag, then it can not be removed</td>
    </tr>
    <tr>
        <td>4</td>
        <td>QUEUE_CREATE_PERM</td>
        <td>10</td>
        <td>.queue_create</td>
        <td>Permission to create queues</td>
    </tr>
    <tr>
        <td>5</td>
        <td>QUEUE_DECLARE_PERM</td>
        <td>11</td>
        <td>.queue_declare</td>
        <td>Permission to declare the queue</td>
    </tr>
    <tr>
        <td>6</td>
        <td>QUEUE_EXIST_PERM</td>
        <td>12</td>
        <td>.queue_exist</td>
        <td>Permission to check the existence of the queue</td>
    </tr>
    <tr>
        <td>7</td>
        <td>QUEUE_LIST_PERM</td>
        <td>13</td>
        <td>.queue_list</td>
        <td>Permission to get a list of queues</td>
    </tr>
    <tr>
        <td>8</td>
        <td>QUEUE_SIZE_PERM</td>
        <td>14</td>
        <td>.queue_size</td>
        <td>Permission to get the size of the queue</td>
    </tr>
    <tr>
        <td>9</td>
        <td>QUEUE_PUSH_PERM</td>
        <td>15</td>
        <td>.queue_push</td>
        <td>Permission to push messages to the queue</td>
    </tr>
    <tr>
        <td>10</td>
        <td>QUEUE_GET_PERM</td>
        <td>16</td>
        <td>.queue_get</td>
        <td>Permission to get messages from the queue</td>
    </tr>
    <tr>
        <td>11</td>
        <td>QUEUE_POP_PERM</td>
        <td>17</td>
        <td>.queue_pop</td>
        <td>Permission to pop messages from the queue</td>
    </tr>
    <tr>
        <td>12</td>
        <td>QUEUE_SUBSCRIBE_PERM</td>
        <td>18</td>
        <td>.queue_subscribe</td>
        <td>Permission to subscribe to the queue</td>
    </tr>
    <tr>
        <td>13</td>
        <td>QUEUE_UNSUBSCRIBE_PERM</td>
        <td>19</td>
        <td>.queue_unsubscribe</td>
        <td>Permission to unsubscribe to the queue</td>
    </tr>
    <tr>
        <td>14</td>
        <td>QUEUE_PURGE_PERM</td>
        <td>20</td>
        <td>.queue_purge</td>
        <td>Permission to delete all messages from the queue</td>
    </tr>
    <tr>
        <td>15</td>
        <td>QUEUE_DELETE_PERM</td>
        <td>21</td>
        <td>.queue_delete</td>
        <td>Permission to delete queue</td>
    </tr>
</table>

Queues
=======
In EagleMQ queues are the main working primitive. Queues are used to store messages and delivering them to customers.
Queues can be used in synchronous or asynchronous mode. In synchronous mode, the client is fully itself manages the queue and are operating under the request -> response.
In asynchronous mode, the server may itself send notifications and messages to the client. To work in asynchronous mode, you must be a subscriber to the queue.

Commands for working with queues
================================
* .queue\_create
* .queue\_declare
* .queue\_exist
* .queue\_list
* .queue\_size
* .queue\_push
* .queue\_get
* .queue\_pop
* .queue\_subscribe
* .queue\_unsubscribe
* .queue\_purge
* .queue\_delete

Description of the commands for working with queues
===================================================
.queue\_create(name, max\_msg, max\_msg\_size, flags)
---------------------------------------------------------------------------------
Command *.queue\_create* creates a queue with the name *name*,
maximum number of messages *max\_msg*,
maximum message size *max\_msg\_size* and flags *flags*.

Message size *max\_msg\_size* can not be greater 2147483647.
If the message size *max\_msg\_size* is 0, then it will be set to 2147483647.

The maximum message size *max\_msg\_size* in bytes.

Flags *flags* are a bit sequence.
Queue supports 4 flags - QUEUE\_AUTODELETE, QUEUE\_FORCE\_PUSH, QUEUE\_ROUND\_ROBIN and QUEUE\_DURABLE.

QUEUE\_AUTODELETE indicates that the queue is deleted automatically if the clients do not use it and it has no subscribers.

QUEUE\_FORCE\_PUSH indicates that if the number of messages in the queue will be equal *max_msg*,
the oldest message can be removed to save the newer one.

QUEUE\_ROUND\_ROBIN indicates that each message will be sent only one subscriber.
For message distribution used algorithm round-robin.

QUEUE\_DURABLE indicates that the queue and the data in the queue will be stored in the storage(according to your settings storage).

Queue name *name* can not have a length greater than 64.

.queue\_declare(name)
--------------------------------
Command *.queue\_declare* declares queue named *name*.

The declaration marks the beginning using the queue.

If the queue is not declared by the client, it is only the following operations on queues:

* .queue\_create
* .queue\_declare
* .queue\_exist
* .queue\_list
* .queue\_size
* .queue\_delete

Queue name *name* can not have a length greater than 64.

.queue\_exist(name)
-----------------------------
Command *.queue\_exist* checks for the existence queue with the name *name*.

Queue name *name* can not have a length greater than 64.

.queue\_list
-----------------
Command *.queue\_list* used to get a list of queues.

The server provides the following information about each queue:

* name - queue name
* max\_msg - maximum number of messages
* max\_msg\_size - maximum message size
* flags - flags with which the queue was created
* size - the number of messages in the queue
* declared clients - the number of clients which declare queue
* subscribed clients - the number of clients subscribed to queue

.queue\_size(name)
----------------------------
Command *.queue\_size* returns the number of messages in the queue with the name *name*.

Queue name *name* can not have a length greater than 64.

.queue\_push(name, message)
---------------------------------------------
Command *.queue\_push* sends *message* to the queue with the name *name*.

Queue name *name* can not have a length greater than 64.

.queue\_get(name)
---------------------------
Command *.queue\_get* gets the most old message that was sent to the queue with the name *name*.

This command does not change the queue.

Queue name *name* can not have a length greater than 64.

.queue\_pop(name)
----------------------------
Command *.queue\_pop* takes the most old message that was sent to the queue with the name *name*.

Queue name *name* can not have a length greater than 64.

.queue\_subscribe(name, flags)
---------------------------------------------
Command *.queue\_subscribe* subscribe client to the queue with the name *name* using flags *flags*.

Subscribe to the queue means notify client after certain event occurred with the queue.
Subscription changes the behavior of the queue.

Flags *flags* are a bit sequence.
Flags *flags* may take two values - QUEUE\_SUBSCRIBE\_MSG or QUEUE\_SUBSCRIBE\_NOTIFY.

If perform subscription with flag QUEUE\_SUBSCRIBE\_MSG,
then after every *.queue\_push* command server automatically sent a message to client that was sent to the queue.
Message is sent to all clients which have subscribed to queue with this flag.
If there is at least one subscriber to queue a flag QUEUE\_SUBSCRIBE\_MSG,
then the message will not be sent to the queue because will be sent directly to the client.

If perform subscription with flag QUEUE\_SUBSCRIBE\_NOTIFY,
then after every *.queue\_push* command server sends a notification to client that
this queue receive a new message.
Notification is sent to all clients who have subscribed to queue with this flag.

Queue name *name* can not have a length greater than 64.

.queue\_unsubscribe(name)
----------------------------------------
Command *.queue\_unsubscribe* removes the client from the list of subscribers to the queue with name *name*.

Queue name *name* can not have a length greater than 64.

.queue\_purge(name)
-------------------------------
Command *.queue\_purge* deletes all messages in a queue with the name *name*.

Queue name *name* can not have a length greater than 64.

.queue\_delete(name)
--------------------------------
Command *.queue\_delete* removes the queue with name *name*.

When you run this command all messages in the queue are deleted.
All clients who have subscribed to the queue deletes from the list of subscribers.

Queue name *name* can not have a length greater than 64.
