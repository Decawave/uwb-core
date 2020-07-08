title SS_TWR with Coex

note over Tag:
    Schedule a callout event using MCU hardware timer
end note
Tag-->+Tag: Callout
Tag-->Tag: Start TX, enable delay start, enable receiver after TX, enable Timeout

note left of Tag:
    Timeout = response frame duration + Hold-off + 1us
end note

deactivate Tag

note over Node:
    Schedule callout event using APs hardware timer
    Callout event timer < RX start time - 1ms
end note
Node-->+Node: Callout
Node-->Node: Toggle Coex
Node-->Node: Start RX, enable delay start, enable Timeout
deactivate Node

note left of Node:
    Timeout = request frame duration + Hold-off + 1us
end note

opt delay start error
Tag-->+Tag: Clear semaphore
deactivate Tag
end

note right of Tag:
    The delay start error is an indication that OS latency
    is significent and the scheduled time has already ellapsed.
    You need to reduce OS latency or schedule an earlier
    callout event
end note

Tag->Node: Request

alt Received Frame Good
deactivate Tag
Node-->+Node: Schedule response, enable delay start
deactivate Node
else delay start error
Node-->+Node: Toggle Coex
Node-->Node: Clear semaphore
deactivate Node
else Timeout
Node-->+Node: Toggle Coex
Node-->Node: Clear semaphore
deactivate Node
end

note over Node: delay start = RX timestamp + request data duration + Hold-off + SHR Len

note right of Tag:
    The delay start error indicates the scheduled time has already elapsed.
    This can be due to context switch latency or execution time.
end note

note right of Tag:
    Hold-off time needs to absorb context switch latency and IRQ execution time
    Hold-off is known apriori to both Tag and Node
    Hold-off can be fixed or negotiated with Information Elements
end note

Node->Tag:Response

alt Transmit Frame Good
Node-->+Node: Toggle Coex
Node-->Node: Clear semaphore
deactivate Node
else TX start error
Node-->+Node:Toggle Coex
Node-->Node: Clear semaphore
deactivate Node
end

alt Receive Frame Good
Tag-->+Tag: Calculate Clock Skew
Tag-->Tag: Calculate ToF
Tag-->Tag: Clear semaphore
deactivate Tag
else Timeout
Tag-->+Tag: Clear semaphore
deactivate Tag
end



