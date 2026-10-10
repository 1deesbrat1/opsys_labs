public class vector {
    
    private double x;
    private double y;
    private double z;

    public vector(double x, double y, double z) {
        this.x = x;
        this.y = y;
        this.z = z;
    } 

    public double getX() {
        return x;
    }

    public double getY() {
        return y;
    }

    public double getZ() {
        return z;
    }

    public void setX(double x) {
        this.x = x;
    }

    public void setY(double y) {
        this.y = y;
    }

    public void setZ(double z) {
        this.z = z;
    }

    public vector sumVector(vector first, vector second) {
        return new vector(
            first.x + second.x,
            first.y + second.y,
            first.z + second.z
        );
    }

    public vector subVector(vector first, vector second) {
        return new vector(
            first.x - second.x,
            first.y - second.y, 
            first.z - second.z
        );
    }

    public vector multiply(vector first, int a) {
        return new vector(
            first.x * a, 
            first.y * a, 
            first.z * a
        );
    }

    public double multiply(vector first, vector second) {
        return first.x * second.x + first.y * second.y + first.z * second.z;
    }
}