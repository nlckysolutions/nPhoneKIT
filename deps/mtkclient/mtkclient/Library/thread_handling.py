def writedata(filename, rq):
    pos = 0
    with open(filename, "wb") as wf:
        while True:
            data = rq.get()
            if data is None:
                break
            pos += len(data)
            wf.write(data)
            rq.task_done()
