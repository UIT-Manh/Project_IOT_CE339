const mongoose = require('mongoose')
const Schema  = mongoose.Schema
const environment = new Schema ({
    temp:{
        type:Number
    },
    hum:{
        type:Number
    },
    smoke:{
        type:Number
    },
    time:{
        type:String
    },
    status:{
        type:Number
    },
    timestamp: { type: Date, default: Date.now }
})
const environmentModel = mongoose.model('Environment', environment)
module.exports= environmentModel