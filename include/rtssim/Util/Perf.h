
template<typename T>
struct AvgValue {
    AvgValue(int sampleCount) : sampleCount(sampleCount) {
        // samples = new T[sampleCount];
    };
    ~AvgValue() {
        // delete[] samples;
    }
    void addSample(const T& t) {
        sumSample = sumSample + t;
        nextSample = (nextSample + 1) % sampleCount;
        if(nextSample == 0){
            avgSample = sumSample / sampleCount;
            sumSample = 0;
        }
    }
    T getAvg() {
        return avgSample;   
    }
private:
    T avgSample{};
    T sumSample{};
    int sampleCount = 0;
    int nextSample = 0;
};