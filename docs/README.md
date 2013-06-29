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
* Routes - number of routes
* Channels - number of channels

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
Flags *flags* may take 3 values - FLUSH\_USER, FLUSH\_QUEUE, FLUSH\_ROUTE.

Flag FLUSH\_USER indicates that deletes all users (except the main administrator).

Flag FLUSH\_QUEUE indicates that deletes all queues.

Flag FLUSH\_ROUTE indicates that deletes all routes.

Flag FLUSH\_CHANNEL indicates that deletes all channels.

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
<table>
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
            <i>.queue_rename</i><br/>
            <i>.queue_size</i><br/>
            <i>.queue_push</i><br/>
            <i>.queue_get</i><br/>
            <i>.queue_pop</i><br/>
            <i>.queue_confirm<i><br/>
            <i>.queue_subscribe</i><br/>
            <i>.queue_unsubscribe</i><br/>
            <i>.queue_purge</i><br/>
            <i>.queue_delete</i>
		</td>
        <td>The ability to perform all the commands associated with queues</td>
    </tr>
    <tr>
        <td>2</td>
        <td>ROUTE_PERM</td>
        <td>1</td>
        <td>
            <i>.route_create</i><br/>
            <i>.route_exist</i><br/>
            <i>.route_list</i><br/>
            <i>.route_keys</i><br/>
            <i>.route_rename</i><br/>
            <i>.route_bind</i><br/>
            <i>.route_unbind</i><br/>
            <i>.route_push</i><br/>
            <i>.route_delete</i>
        </td>
        <td>The ability to perform all the commands associated with routes</td>
    </tr>
    <tr>
        <td>3</td>
        <td>CHANNEL_PERM</td>
        <td>2</td>
        <td>
            <i>.channel_create</i><br/>
            <i>.channel_exist</i><br/>
            <i>.channel_list</i><br/>
            <i>.channel_rename</i><br/>
            <i>.channel_publish</i><br/>
            <i>.channel_subscribe</i><br/>
            <i>.channel_psubscribe</i><br/>
            <i>.channel_unsubscribe</i><br/>
            <i>.channel_punsubscribe</i><br>
            <i>.channel_delete</i>
        </td>
        <td>The ability to perform all the commands associated with channels</td>
    </tr>
    <tr>
        <td>4</td>
        <td>ADMIN_PERM</td>
        <td>5</td>
        <td>-</td>
        <td>The ability to perform all the commands on the server (including the commands for working with users)</td>
    </tr>
    <tr>
        <td>5</td>
        <td>NOT_CHANGE_PERM</td>
        <td>6</td>
        <td>-</td>
        <td>If a user is created with the flag, then it can not be removed</td>
    </tr>
    <tr>
        <td>6</td>
        <td>QUEUE_CREATE_PERM</td>
        <td>20</td>
        <td>.queue_create</td>
        <td>Permission to create queue</td>
    </tr>
    <tr>
        <td>7</td>
        <td>QUEUE_DECLARE_PERM</td>
        <td>21</td>
        <td>.queue_declare</td>
        <td>Permission to declare the queue</td>
    </tr>
    <tr>
        <td>8</td>
        <td>QUEUE_EXIST_PERM</td>
        <td>22</td>
        <td>.queue_exist</td>
        <td>Permission to check the existence of the queue</td>
    </tr>
    <tr>
        <td>9</td>
        <td>QUEUE_LIST_PERM</td>
        <td>23</td>
        <td>.queue_list</td>
        <td>Permission to get a list of queues</td>
    </tr>
    <tr>
        <td>10</td>
        <td>QUEUE_RENAME_PERM</td>
        <td>24</td>
        <td>.queue_rename</td>
        <td>Permission to rename the queue</td>
    </tr>
    <tr>
        <td>11</td>
        <td>QUEUE_SIZE_PERM</td>
        <td>25</td>
        <td>.queue_size</td>
        <td>Permission to get the size of the queue</td>
    </tr>
    <tr>
        <td>12</td>
        <td>QUEUE_PUSH_PERM</td>
        <td>26</td>
        <td>.queue_push</td>
        <td>Permission to push messages to the queue</td>
    </tr>
    <tr>
        <td>13</td>
        <td>QUEUE_GET_PERM</td>
        <td>27</td>
        <td>.queue_get</td>
        <td>Permission to get messages from the queue</td>
    </tr>
    <tr>
        <td>14</td>
        <td>QUEUE_POP_PERM</td>
        <td>28</td>
        <td>.queue_pop</td>
        <td>Permission to pop messages from the queue</td>
    </tr>
    <tr>
        <td>15</td>
        <td>QUEUE_CONFIRM_PERM</td>
        <td>29</td>
        <td>.queue_confirm</td>
        <td>Permission for confirmation message from the queue</td>
    </tr>
    <tr>
        <td>16</td>
        <td>QUEUE_SUBSCRIBE_PERM</td>
        <td>30</td>
        <td>.queue_subscribe</td>
        <td>Permission to subscribe to the queue</td>
    </tr>
    <tr>
        <td>17</td>
        <td>QUEUE_UNSUBSCRIBE_PERM</td>
        <td>31</td>
        <td>.queue_unsubscribe</td>
        <td>Permission to unsubscribe to the queue</td>
    </tr>
    <tr>
        <td>18</td>
        <td>QUEUE_PURGE_PERM</td>
        <td>32</td>
        <td>.queue_purge</td>
        <td>Permission to delete all messages from the queue</td>
    </tr>
    <tr>
        <td>19</td>
        <td>QUEUE_DELETE_PERM</td>
        <td>33</td>
        <td>.queue_delete</td>
        <td>Permission to delete queue</td>
    </tr>
    <tr>
        <td>20</td>
        <td>ROUTE_CREATE_PERM</td>
        <td>34</td>
        <td>.route_create</td>
        <td>Permission to create route</td>
    </tr>
    <tr>
        <td>21</td>
        <td>ROUTE_EXIST_PERM</td>
        <td>35</td>
        <td>.route_exist</td>
        <td>Permission to check the existence of the route</td>
    </tr>
    <tr>
        <td>22</td>
        <td>ROUTE_LIST_PERM</td>
        <td>36</td>
        <td>.route_list</td>
        <td>Permission to get a list of routes</td>
    </tr>
    <tr>
        <td>23</td>
        <td>ROUTE_KEYS_PERM</td>
        <td>37</td>
        <td>.route_keys</td>
        <td>Permission to get a list of keys route</td>
    </tr>
    <tr>
        <td>24</td>
        <td>ROUTE_RENAME_PERM</td>
        <td>38</td>
        <td>.route_rename</td>
        <td>Permission to rename the route</td>
    </tr>
    <tr>
        <td>25</td>
        <td>ROUTE_BIND_PERM</td>
        <td>39</td>
        <td>.route_bind</td>
        <td>Permission to bind route with the queue</td>
    </tr>
    <tr>
        <td>26</td>
        <td>ROUTE_UNBIND_PERM</td>
        <td>40</td>
        <td>.route_unbind</td>
        <td>Permission to unbind route from the queue</td>
    </tr>
    <tr>
        <td>27</td>
        <td>ROUTE_PUSH_PERM</td>
        <td>41</td>
        <td>.route_push</td>
        <td>Permission to push messages to the route</td>
    </tr>
    <tr>
        <td>28</td>
        <td>ROUTE_DELETE_PERM</td>
        <td>42</td>
        <td>.route_delete</td>
        <td>Permission to delete route</td>
    </tr>
    <tr>
        <td>29</td>
        <td>CHANNEL_CREATE_PERM</td>
        <td>43</td>
        <td>.channel_create</td>
        <td>Permission to create channel</td>
    </tr>
    <tr>
        <td>30</td>
        <td>CHANNEL_EXIST_PERM</td>
        <td>44</td>
        <td>.channel_exist</td>
        <td>Permission to check the existence of the channel</td>
    </tr>
    <tr>
        <td>31</td>
        <td>CHANNEL_LIST_PERM</td>
        <td>45</td>
        <td>.channel_list</td>
        <td>Permission to get a list of channel</td>
    </tr>
    <tr>
        <td>32</td>
        <td>CHANNEL_RENAME_PERM</td>
        <td>46</td>
        <td>.channel_rename</td>
        <td>Permission to rename the channel</td>
    </tr>
    <tr>
        <td>33</td>
        <td>CHANNEL_PUBLISH_PERM</td>
        <td>47</td>
        <td>.channel_publish</td>
        <td>Permission to publish message to the channel</td>
    </tr>
    <tr>
        <td>34</td>
        <td>CHANNEL_SUBSCRIBE_PERM</td>
        <td>48</td>
        <td>.channel_subscribe</td>
        <td>Permission to subscribe to the channel by topic</td>
    </tr>
    <tr>
        <td>35</td>
        <td>CHANNEL_PSUBSCRIBE_PERM</td>
        <td>49</td>
        <td>.channel_psubscribe</td>
        <td>Permission to subscribe to the channel by pattern</td>
    </tr>
    <tr>
        <td>36</td>
        <td>CHANNEL_UNSUBSCRIBE_PERM</td>
        <td>50</td>
        <td>.channel_unsubscribe</td>
        <td>Permission to unsubscribe from the channel by topic</td>
    </tr>
    <tr>
        <td>37</td>
        <td>CHANNEL_PUNSUBSCRIBE_PERM</td>
        <td>51</td>
        <td>.channel_punsubscribe</td>
        <td>Permission to unsubscribe from the channel by pattern</td>
    </tr>
    <tr>
        <td>38</td>
        <td>CHANNEL_DELETE_PERM</td>
        <td>52</td>
        <td>.channel_delete</td>
        <td>Permission to delete channel</td>
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
* .queue\_rename
* .queue\_size
* .queue\_push
* .queue\_get
* .queue\_pop
* .queue\_confirm
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

QUEUE\_DURABLE indicates that the queue and the data in the queue will be stored in the storage (according to your settings storage).

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
* .queue\_rename
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
* flags - flags that created queue
* size - the number of messages in the queue
* declared clients - the number of clients which declare queue
* subscribed clients - the number of clients subscribed to queue

.queue\_rename(from, to)
----------------------------------
Command *.queue\_rename* renames a queue named *from* in *to*.

Queue name *from* and *to* can not have a length greater than 64.

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

.queue\_pop(name, timeout)
----------------------------
Command *.queue\_pop* takes the most old message that was sent to the queue with the name *name*.

*timeout* indicates confirm delivery timeout in milliseconds. If the message is not confirmed for the specified time, the message will be returned to the queue as the oldest.

If *timeout* is 0, message delivery confirmation is not needed.

Queue name *name* can not have a length greater than 64.

.queue\_confirm(name, tag)
--------------------------------
Command *.queue\_confirm* confirms the message delivery.

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

Routes
======
Routes allow you to distribute incoming messages to the queues.
For distribution the messages you must to bind queue with the key and when sending specify key.

Commands for working with routes
===============================
* .route\_create
* .route\_exist
* .route\_list
* .route\_keys
* .route\_rename
* .route\_bind
* .route\_unbind
* .route\_push
* .route\_delete

Description of the commands for working with routes
===================================================
.route\_create(name, flags)
---------------------------
Command *.route\_create* creates a route with name *name* and flags *flags*.

Flags *flags* are a bit sequence. Route supports 3 flags - ROUTE\_AUTODELETE, ROUTE\_ROUND\_ROBIN and ROUTE\_DURABLE.

ROUTE\_AUTODELETE indicates that the route will be automatically removed if none queue is not binded with it.

ROUTE\_ROUND\_ROBIN indicates that each message will be sent only one queue binded by key.
For message distribution used algorithm round-robin.

ROUTE\_DURABLE indicates that the route will be stored in the storage (according to your settings storage).

Route name *name* can not have a length greater than 64.

.route\_exist(name)
-------------------
Command *.route_exist* checks for the existence route with the name *name*.

Route name *name* can not have a length greater than 64.

.route\_list
------------
Command *.route_list* used to get list of routes.

The server provides the following information about each route:

* name - name of the route
* flags - flags that created route
* keys - the number of keys in the route

.route\_keys(name)
------------------
Command *.route_keys* gets a list of all keys of route with the name *name*.

The server provides the following information about each key:

* key - the key
* queue - binded queue

Route name *name* can not have a length greater than 64.

.route\_rename(from, to)
----------------------------------
Command *.route\_rename* renames a route named *from* in *to*.

Route name *from* and *to* can not have a length greater than 64.

.route\_bind(name, queue, key)
------------------------------
Command *.route\_bind* binds route with the name *name* to queue *queue* by a key *key*.

Route name *name* can not have a length greater than 64.

Queue name *name* can not have a length greater than 64.

Key *key* can not have a length greater than 32.

.route\_unbind(name, queue, key)
--------------------------------
Command *.route\_unbind* unbinds route with the name *name* from queue *queue* by a key *key*.

Route name *name* can not have a length greater than 64.

Queue name *name* can not have a length greater than 64.

Key *key* can not have a length greater than 32.

.route\_push(name, key, message)
--------------------------------
Command *.route\_push* sends *message* to the route with the name *name* by a key *key*.

Route name *name* can not have a length greater than 64.

Key *key* can not have a length greater than 32.

.route\_delete(name)
--------------------
Command *.route\_delete* removes the route with name *name*.

When you run this command all connections with the queues are deleted.

Route name *name* can not have a length greater than 64.

Channels
========
Channels are used to send messages to clients.

Channels will never retain the message. The message can be sent to all clients who listen the channel on the topic or pattern.

Topic - some label on which you can send a message and it will be delivered to all subscribed on the label clients.

Pattern - is used to subscribe to several topics at once. The format of the template is to glob-style pattern.


Commands for working with channels
==================================
* .channel\_create
* .channel\_exist
* .channel\_list
* .channel\_rename
* .channel\_publish
* .channel\_subscribe
* .channel\_psubscribe
* .channel\_unsubscribe
* .channel\_punsubscribe
* .channel\_delete

Description of the commands for working with channels
=====================================================
.channel\_create(name, flags)
----------------------------
Command *.channel\_create* creates a channel with name *name* and flags *flags*.

Flags *flags* are a bit sequence. Channel supports 3 flags - CHANNEL\_AUTODELETE, CHANNEL\_ROUND\_ROBIN and CHANNEL\_DURABLE.

CHANNEL\_AUTODELETE indicates that the channel will be automatically removed if it has no subscriptions.

CHANNEL\_ROUND\_ROBIN indicates that each message will be sent only to one subscriber by topic. On the subscription pattern this flag is not affected.
For message distribution used algorithm round-robin.

CHANNEL\_DURABLE indicates that the channel will be stored in the storage (according to your settings storage).

Channel name *name* can not have a length greater than 64.

.channel\_exist(name)
--------------------
Command *.channel\_exist* checks for the existence of a channel with the name *name*.

Channel name *name* can not have a length greater than 64.

.channel\_list
--------------
Command *.channel\_list* is used to get list of channels.

The server provides the following information for each channel:

* name - the name of the channel
* flags - flags that created channel
* topics - number of topics in the channel
* patterns - the number of patterns in the channel

.channel\_rename(from, to)
--------------------------
Command *.channel\_rename* renames a channel named *from* in *to*.

Channel name *from* and *to* can not have a length greater than 64.

.channel\_publish(name, topic, message)
--------------------------------------
Command *.channel\_publish* sends *message* to the channel with the name *name* using the topic *topic*.

Channel name *name* can not have a length greater than 64.

Topic name *topic* can not have a length greater than 32.

.channel\_subscribe(name, topic)
--------------------------------
Command *.channel\_subscribe* subscribe client to the channel with the name *name* using the topic *topic*.

Channel name *name* can not have a length greater than 64.

Topic *topic* can not have a length greater than 32.

.channel\_psubscribe(name, pattern)
-----------------------------------
Command *.channel\_psubscribe* subscribe client to the channel with the name *name* using the pattern *pattern*.

Channel name *name* can not have a length greater than 64.

Pattern *pattern* can not have a length greater than 32.

.channel\_unsubscribe(name, topic)
----------------------------------
Command *.channel\_unsubscribe* removes the client from the list of subscribers to the channel with the name *name* and topic *topic*.

Channel name *name* can not have a length greater than 64.

Topic *topic* can not have a length greater than 32.

.channel\_punsubscribe(name, pattern)
-------------------------------------
Command *.channel\_punsubscribe* removes the client from the list of subscribers to the channel with the name *name* and pattern *pattern*.

Channel name *name* can not have a length greater than 64.

Pattern *pattern* can not have a length greater than 32.

.channel\_delete(name)
----------------------
Command *.channel\_delete* removes the channel with name *name*.

Channel name *name* can not have a length greater than 64.
